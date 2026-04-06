#pragma once

#include <Arduino.h>

class ConfigManager;
class AudioManager;
class NtpSync;
class PrayerScheduler;

class MultiRoomSync {
public:
    void begin(ConfigManager* config, AudioManager* audio,
               NtpSync* ntp, PrayerScheduler* scheduler);
    void update();

    // Set a sync trigger from cloud or local API
    void setSyncTrigger(int prayerIndex, unsigned long triggerEpoch);

    bool isInGroup() const;
    bool hasPendingTrigger() const { return _pendingTrigger; }

private:
    ConfigManager* _config = nullptr;
    AudioManager* _audio = nullptr;
    NtpSync* _ntp = nullptr;
    PrayerScheduler* _scheduler = nullptr;

    bool _pendingTrigger = false;
    int _triggerPrayer = -1;
    unsigned long _triggerEpoch = 0;
};
