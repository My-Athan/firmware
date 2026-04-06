#include "PrayerScheduler.h"
#include "../config/ConfigManager.h"
#include "../audio/AudioManager.h"
#include "../led/LedManager.h"
#include "../time/NtpSync.h"
#include "../config/defaults.h"

// ─────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::begin(ConfigManager* config, AudioManager* audio,
                            LedManager* led, NtpSync* ntp, IqamaTimer* iqama) {
    _config = config;
    _audio = audio;
    _led = led;
    _ntp = ntp;
    _iqama = iqama;

    _holiday.begin(config);

    // Configure calculator from config
    _calculator.setMethodFromString(config->getCalcMethod());
    _calculator.setAsrFromString(config->getAsrJuristic());
    _calculator.setHighLatFromString(config->getHighLatitudeRule());

    _restoreRecovery();

    Serial.println("[Scheduler] Initialized");
}

// ─────────────────────────────────────────────────────────────
// Main tick — called every loop iteration
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::tick() {
    if (!_ntp || !_ntp->isSynced()) return;

    // Throttle checks to every 10 seconds
    unsigned long now = millis();
    if (now - _lastTickMs < 10000) return;
    _lastTickMs = now;

    int currentDay = _ntp->currentDay();
    int nowMinutes = _ntp->minutesSinceMidnight();
    int dayOfWeek = _ntp->currentDayOfWeek();

    // Recalculate prayer times at midnight or on first run
    if (currentDay != _lastCalcDay) {
        _recalculate();
        _lastCalcDay = currentDay;
        _playedTodayMask = 0;  // Reset daily
        _config->setPlayedTodayMask(0);

        // Check Hijri date / holidays
        _holiday.checkDate(_ntp->currentYear(), _ntp->currentMonth(), currentDay);
    }

    // State machine
    switch (_state) {
        case SchedulerState::IDLE:
            _checkPreAthan(nowMinutes);
            _checkPrayerTimes(nowMinutes, dayOfWeek);
            if (_holiday.isRamadan() && _config->isRamadanModeEnabled()) {
                _checkSuhoorAlert(nowMinutes);
            }
            break;

        case SchedulerState::PRE_ATHAN:
            _checkPrayerTimes(nowMinutes, dayOfWeek);
            break;

        case SchedulerState::PLAYING_ATHAN:
            if (_audio && _audio->isIdle()) {
                _onAthanFinished();
            }
            break;

        case SchedulerState::POST_ATHAN:
            if (_audio && _audio->isIdle()) {
                // Post-athan track finished
                if (_iqama) {
                    _iqama->startCountdown(_currentPrayerIndex);
                }
                _state = SchedulerState::IDLE;
                if (_led) _led->setState(LedState::IDLE);
            }
            break;

        case SchedulerState::IQAMA_WAIT:
            // IqamaTimer handles its own state
            if (_iqama && !_iqama->isActive()) {
                _state = SchedulerState::IDLE;
            }
            break;
    }
}

// ─────────────────────────────────────────────────────────────
// Prayer time calculation
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::_recalculate() {
    float lat = _config->getLatitude();
    float lon = _config->getLongitude();

    if (lat == 0.0f && lon == 0.0f) {
        Serial.println("[Scheduler] Location not set, skipping calculation");
        return;
    }

    int year = _ntp->currentYear();
    int month = _ntp->currentMonth();
    int day = _ntp->currentDay();

    _todayTimes = _calculator.calculate(year, month, day, lat, lon);

    // Apply offsets
    _todayTimes.fajr    += _config->getPrayerOffset(PRAYER_FAJR);
    _todayTimes.dhuhr   += _config->getPrayerOffset(PRAYER_DHUHR);
    _todayTimes.asr     += _config->getPrayerOffset(PRAYER_ASR);
    _todayTimes.maghrib += _config->getPrayerOffset(PRAYER_MAGHRIB);
    _todayTimes.isha    += _config->getPrayerOffset(PRAYER_ISHA);

    // Calculate tomorrow for suhoor calculation
    // Simple: just add 1 day (good enough for suhoor lookahead)
    struct tm t = _ntp->localTime();
    t.tm_mday += 1;
    mktime(&t);  // Normalize
    _tomorrowTimes = _calculator.calculate(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, lat, lon);

    char buf[6];
    Serial.println("[Scheduler] Prayer times calculated:");
    const char* names[] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};
    int times[] = {_todayTimes.fajr, _todayTimes.sunrise, _todayTimes.dhuhr,
                   _todayTimes.asr, _todayTimes.maghrib, _todayTimes.isha};
    for (int i = 0; i < 6; i++) {
        formatTime(times[i], buf, sizeof(buf));
        Serial.printf("  %s: %s\n", names[i], buf);
    }
}

// ─────────────────────────────────────────────────────────────
// Prayer time checks
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::_checkPrayerTimes(int nowMinutes, int dayOfWeek) {
    for (int i = 0; i < PRAYER_COUNT; i++) {
        // Skip if already played today
        if (_playedTodayMask & (1 << i)) continue;

        // Skip if prayer is disabled
        if (!_config->isPrayerEnabled(i)) continue;

        int prayerTime = _getPrayerMinutes(_todayTimes, i);
        if (prayerTime <= 0) continue;

        // Check if it's time (within 1-minute window)
        if (nowMinutes >= prayerTime && nowMinutes < prayerTime + 2) {
            _playAthan(i, dayOfWeek);
            return;  // One prayer at a time
        }
    }
}

void PrayerScheduler::_checkSuhoorAlert(int nowMinutes) {
    // Suhoor alert fires N minutes before Fajr
    int suhoorMinutes = _config->getSuhoorAlertMinutes();
    if (suhoorMinutes <= 0) return;

    int fajrTime = _todayTimes.fajr;
    int suhoorTime = fajrTime - suhoorMinutes;
    if (suhoorTime < 0) suhoorTime += 24 * 60;

    // Check if already played (use bit 5 for suhoor)
    if (_playedTodayMask & (1 << 5)) return;

    if (nowMinutes >= suhoorTime && nowMinutes < suhoorTime + 2) {
        const char* mode = _config->getSuhoorMode();
        Serial.printf("[Scheduler] Suhoor alert! Mode: %s\n", mode);

        _playedTodayMask |= (1 << 5);

        if (strcmp(mode, "none") == 0) return;

        if (strcmp(mode, "led") == 0 || _config->isSuhoorLedEnabled()) {
            if (_led) _led->setState(LedState::PRE_ATHAN);
        }

        if (strcmp(mode, "sound") == 0 || strcmp(mode, "custom") == 0) {
            int track = _config->getSuhoorTrack();
            if (track > 0 && _audio) {
                _audio->setVolume(_config->getVolume());
                _audio->play(track);
            }
        }
    }
}

void PrayerScheduler::_checkPreAthan(int nowMinutes) {
    int preMinutes = _config->getPreAthanMinutes();
    if (preMinutes <= 0) return;

    for (int i = 0; i < PRAYER_COUNT; i++) {
        if (_playedTodayMask & (1 << i)) continue;
        int prayerTime = _getPrayerMinutes(_todayTimes, i);
        if (prayerTime <= 0) continue;

        int preTime = prayerTime - preMinutes;
        if (nowMinutes >= preTime && nowMinutes < prayerTime) {
            if (_state != SchedulerState::PRE_ATHAN) {
                _state = SchedulerState::PRE_ATHAN;
                if (_led) _led->setState(LedState::PRE_ATHAN);
            }
            return;
        }
    }

    // No pre-athan active
    if (_state == SchedulerState::PRE_ATHAN) {
        _state = SchedulerState::IDLE;
        if (_led) _led->setState(LedState::IDLE);
    }
}

// ─────────────────────────────────────────────────────────────
// Athan playback
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::_playAthan(int prayerIndex, int dayOfWeek) {
    _currentPrayerIndex = prayerIndex;
    _playedTodayMask |= (1 << prayerIndex);
    _config->setPlayedTodayMask(_playedTodayMask);

    // Determine track
    int track = _config->getTrackForPrayer(prayerIndex);

    // Friday Jumuah: replace Dhuhr with Jumuah track
    if (prayerIndex == PRAYER_DHUHR && dayOfWeek == 5 && _config->isFridayJumuahEnabled()) {
        int jumuahTrack = _config->getJumuahTrack();
        if (jumuahTrack > 0) {
            track = jumuahTrack;
            Serial.println("[Scheduler] Friday Jumuah — using Jumuah track");
        }
    }

    // Ramadan: use special Fajr track if available
    if (_holiday.isRamadan() && _config->isRamadanModeEnabled()) {
        int ramadanTrack = _config->getRamadanTrack(prayerIndex);
        if (ramadanTrack > 0) {
            track = ramadanTrack;
            Serial.println("[Scheduler] Ramadan — using special track");
        }
    }

    // Set per-prayer volume
    int perPrayerVol = _config->getVolumeForPrayer(prayerIndex);
    int globalVol = _config->getVolume();
    if (_audio) {
        _audio->setVolumeForPrayer(prayerIndex, globalVol, perPrayerVol);
    }

    // Check for post-athan track (holiday)
    _postAthanTrack = _holiday.getPostAthanTrack();

    // Play athan
    char timeBuf[6];
    formatTime(_getPrayerMinutes(_todayTimes, prayerIndex), timeBuf, sizeof(timeBuf));
    Serial.printf("[Scheduler] Playing athan for prayer %d at %s (track %d)\n",
                  prayerIndex, timeBuf, track);

    _state = SchedulerState::PLAYING_ATHAN;
    if (_led) _led->setState(LedState::PLAYING);
    if (_audio) _audio->play(track);

    _persistRecovery();
}

void PrayerScheduler::_onAthanFinished() {
    Serial.printf("[Scheduler] Athan finished for prayer %d\n", _currentPrayerIndex);

    // Play post-athan track if holiday (e.g., Eid takbeer)
    if (_postAthanTrack > 0) {
        Serial.printf("[Scheduler] Playing post-athan track %d\n", _postAthanTrack);
        _state = SchedulerState::POST_ATHAN;
        if (_audio) _audio->play(_postAthanTrack);
        return;
    }

    // Check for doaa
    if (_config->isDoaaEnabled(_currentPrayerIndex)) {
        int doaaTrack = _config->getDoaaTrack(_currentPrayerIndex);
        if (doaaTrack > 0) {
            Serial.printf("[Scheduler] Playing doaa track %d\n", doaaTrack);
            _state = SchedulerState::POST_ATHAN;
            if (_audio) _audio->play(doaaTrack);
            return;
        }
    }

    // Start iqama countdown
    if (_iqama) {
        _iqama->startCountdown(_currentPrayerIndex);
        if (_iqama->isActive()) {
            _state = SchedulerState::IQAMA_WAIT;
            return;
        }
    }

    _state = SchedulerState::IDLE;
    if (_led) _led->setState(LedState::IDLE);
    _config->setRecoveryState("idle", -1, 0);
    _config->save();
}

// ─────────────────────────────────────────────────────────────
// Manual triggers
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::triggerAthan(int prayerIndex) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return;
    int dayOfWeek = _ntp ? _ntp->currentDayOfWeek() : 0;
    _playAthan(prayerIndex, dayOfWeek);
}

void PrayerScheduler::triggerPreview(int track) {
    if (_audio) _audio->playPreview(track);
}

// ─────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────

int PrayerScheduler::_getPrayerMinutes(const PrayerTimes& times, int index) const {
    switch (index) {
        case PRAYER_FAJR:    return times.fajr;
        case PRAYER_DHUHR:   return times.dhuhr;
        case PRAYER_ASR:     return times.asr;
        case PRAYER_MAGHRIB: return times.maghrib;
        case PRAYER_ISHA:    return times.isha;
        default: return 0;
    }
}

int PrayerScheduler::getNextPrayerIndex() const {
    if (!_ntp || !_ntp->isSynced()) return -1;
    int now = _ntp->minutesSinceMidnight();

    for (int i = 0; i < PRAYER_COUNT; i++) {
        int t = _getPrayerMinutes(_todayTimes, i);
        if (t > now) return i;
    }
    return PRAYER_FAJR;  // Next is tomorrow's Fajr
}

int PrayerScheduler::getNextPrayerMinutes() const {
    int idx = getNextPrayerIndex();
    if (idx < 0) return 0;
    int t = _getPrayerMinutes(_todayTimes, idx);
    int now = _ntp ? _ntp->minutesSinceMidnight() : 0;
    int diff = t - now;
    if (diff < 0) diff += 24 * 60;
    return diff;
}

void PrayerScheduler::formatTime(int minutes, char* buf, size_t len) {
    int h = (minutes / 60) % 24;
    int m = minutes % 60;
    snprintf(buf, len, "%02d:%02d", h, m);
}

// ─────────────────────────────────────────────────────────────
// Recovery persistence
// ─────────────────────────────────────────────────────────────

void PrayerScheduler::_persistRecovery() {
    _config->setRecoveryState("playing", _currentPrayerIndex, millis());
    _config->save();
}

void PrayerScheduler::_restoreRecovery() {
    const char* lastState = _config->getLastState();
    int lastPrayer = _config->getLastPrayerPlayed();

    if (strcmp(lastState, "idle") == 0 || lastPrayer < 0) return;

    // Power-loss recovery: skip and log (user decision)
    Serial.printf("[Scheduler] Recovery: was %s prayer %d — skipping (power-loss policy)\n",
                  lastState, lastPrayer);
    _config->setRecoveryState("idle", -1, 0);
    _config->save();
}
