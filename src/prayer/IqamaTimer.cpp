#include "IqamaTimer.h"
#include "../config/ConfigManager.h"
#include "../audio/AudioManager.h"
#include "../led/LedManager.h"

void IqamaTimer::begin(ConfigManager* config, AudioManager* audio, LedManager* led) {
    _config = config;
    _audio = audio;
    _led = led;
}

void IqamaTimer::update() {
    if (_state == IqamaState::IDLE) return;

    if (_state == IqamaState::COUNTDOWN) {
        int remaining = getRemainingSeconds();

        // Update LED with countdown progress
        if (_led) {
            _led->setState(LedState::IQAMA_COUNTDOWN);
            _led->setIqamaRemaining(_totalSeconds, remaining);
        }

        if (remaining <= 0) {
            // Countdown expired — play iqama
            Serial.printf("[Iqama] Countdown finished for prayer %d, playing iqama\n", _prayerIndex);
            _state = IqamaState::PLAYING_IQAMA;

            if (_audio && _iqamaTrack > 0) {
                _audio->play(_iqamaTrack);
            } else {
                // No iqama track configured — just finish
                _state = IqamaState::IDLE;
                if (_led) _led->setState(LedState::IDLE);
            }
        }
        return;
    }

    if (_state == IqamaState::PLAYING_IQAMA) {
        if (_audio && _audio->isIdle()) {
            Serial.println("[Iqama] Iqama playback complete");
            _state = IqamaState::IDLE;
            if (_led) _led->setState(LedState::IDLE);
        }
    }
}

void IqamaTimer::startCountdown(int prayerIndex) {
    if (!_config) return;

    int delayMin = _config->getIqamaDelay(prayerIndex);
    if (delayMin <= 0) {
        Serial.printf("[Iqama] No iqama delay configured for prayer %d\n", prayerIndex);
        return;
    }

    _prayerIndex = prayerIndex;
    _totalSeconds = delayMin * 60;
    _startTime = millis();
    _iqamaTrack = _config->getIqamaTrack(prayerIndex);
    _state = IqamaState::COUNTDOWN;

    Serial.printf("[Iqama] Countdown started: %d min for prayer %d (track %d)\n",
                  delayMin, prayerIndex, _iqamaTrack);
}

void IqamaTimer::cancel() {
    if (_state == IqamaState::IDLE) return;
    Serial.println("[Iqama] Cancelled");
    _state = IqamaState::IDLE;
    if (_led) _led->setState(LedState::IDLE);
}

int IqamaTimer::getRemainingSeconds() const {
    if (_state == IqamaState::IDLE) return 0;
    unsigned long elapsed = (millis() - _startTime) / 1000;
    int remaining = _totalSeconds - (int)elapsed;
    return (remaining > 0) ? remaining : 0;
}

void IqamaTimer::restoreState(int prayerIndex, unsigned long startTimestamp) {
    if (!_config) return;

    int delayMin = _config->getIqamaDelay(prayerIndex);
    if (delayMin <= 0) return;

    // Calculate how much time has passed since the original start
    unsigned long nowMs = millis();
    unsigned long elapsedSinceReboot = nowMs;  // Approximate

    _prayerIndex = prayerIndex;
    _totalSeconds = delayMin * 60;
    _iqamaTrack = _config->getIqamaTrack(prayerIndex);

    // Estimate remaining time (imprecise after reboot, but better than nothing)
    _startTime = nowMs - (elapsedSinceReboot);  // Best effort
    _state = IqamaState::COUNTDOWN;

    Serial.printf("[Iqama] Restored countdown for prayer %d\n", prayerIndex);
}
