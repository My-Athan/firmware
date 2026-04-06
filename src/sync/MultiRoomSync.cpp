#include "MultiRoomSync.h"
#include "../config/ConfigManager.h"
#include "../audio/AudioManager.h"
#include "../time/NtpSync.h"
#include "../prayer/PrayerScheduler.h"
#include "../config/defaults.h"

void MultiRoomSync::begin(ConfigManager* config, AudioManager* audio,
                          NtpSync* ntp, PrayerScheduler* scheduler) {
    _config = config;
    _audio = audio;
    _ntp = ntp;
    _scheduler = scheduler;

    if (isInGroup()) {
        Serial.printf("[MultiRoom] In group: %s\n", _config->getMultiRoomGroupId());
    } else {
        Serial.println("[MultiRoom] Standalone mode");
    }
}

void MultiRoomSync::update() {
    if (!_pendingTrigger || !_ntp || !_ntp->isSynced()) return;

    time_t now = _ntp->now();

    // Check if it's time to trigger
    long diff = (long)_triggerEpoch - (long)now;

    if (diff <= 0) {
        if (diff > -2) {
            // Within 2-second window — trigger synchronized playback
            Serial.printf("[MultiRoom] Sync trigger! Prayer %d (%.1fs %s)\n",
                          _triggerPrayer, (float)abs(diff), diff < 0 ? "late" : "early");

            if (_scheduler) {
                _scheduler->triggerAthan(_triggerPrayer);
            }
        } else {
            // Missed by more than 2 seconds — play anyway
            Serial.printf("[MultiRoom] Missed sync by %lds, playing anyway\n", -diff);
            if (_scheduler) {
                _scheduler->triggerAthan(_triggerPrayer);
            }
        }

        _pendingTrigger = false;
        _triggerPrayer = -1;
        _triggerEpoch = 0;
    }
}

void MultiRoomSync::setSyncTrigger(int prayerIndex, unsigned long triggerEpoch) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return;

    _triggerPrayer = prayerIndex;
    _triggerEpoch = triggerEpoch;
    _pendingTrigger = true;

    time_t now = _ntp ? _ntp->now() : 0;
    Serial.printf("[MultiRoom] Sync scheduled: prayer %d at epoch %lu (in %lds)\n",
                  prayerIndex, triggerEpoch, (long)(triggerEpoch - now));
}

bool MultiRoomSync::isInGroup() const {
    if (!_config) return false;
    const char* group = _config->getMultiRoomGroupId();
    return group && strlen(group) > 0;
}
