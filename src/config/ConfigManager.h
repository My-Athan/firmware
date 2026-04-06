#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

#define CONFIG_PATH "/config.json"
#define CONFIG_MAX_SIZE 8192

class ConfigManager {
public:
    bool begin();
    bool load();
    bool save();
    bool reset();

    // ── Generic ─────────────────────────────────────────────
    const char* getDeviceId();
    int getConfigVersion();

    // ── Audio ───────────────────────────────────────────────
    int getVolume();
    int getTrackForPrayer(int prayerIndex);
    int getVolumeForPrayer(int prayerIndex);    // Per-prayer volume (0 = use global)
    bool isPrayerEnabled(int prayerIndex);
    bool isDoaaEnabled(int prayerIndex);
    int getDoaaTrack(int prayerIndex);
    int getDoaaDelay(int prayerIndex);

    // ── Iqama ───────────────────────────────────────────────
    int getIqamaDelay(int prayerIndex);         // Minutes, 0 = disabled
    int getIqamaTrack(int prayerIndex);

    // ── Location / Calculation ──────────────────────────────
    float getLatitude();
    float getLongitude();
    const char* getTimezone();
    const char* getCalcMethod();
    const char* getAsrJuristic();
    const char* getHighLatitudeRule();

    // ── Schedule ────────────────────────────────────────────
    bool isFridayJumuahEnabled();
    int getJumuahTrack();

    // ── Ramadan ─────────────────────────────────────────────
    bool isRamadanModeEnabled();
    int getSuhoorAlertMinutes();
    const char* getSuhoorMode();
    int getSuhoorTrack();
    bool isSuhoorLedEnabled();
    bool isIftarAlertEnabled();
    int getIftarTrack();
    int getRamadanTrack(int prayerIndex);

    // ── Hijri ───────────────────────────────────────────────
    int getHijriAdjustment();

    // ── Holidays ────────────────────────────────────────────
    bool isHolidayEnabled(const char* holidayKey);
    int getHolidayTrack(const char* holidayKey);
    int getHolidayPostAthanTrack(const char* holidayKey);

    // ── LED ─────────────────────────────────────────────────
    bool isLedEnabled();
    int getPreAthanMinutes();

    // ── Multi-room ──────────────────────────────────────────
    const char* getMultiRoomGroupId();
    int getMultiRoomSyncOffset();

    // ── Recovery ────────────────────────────────────────────
    const char* getLastState();
    int getLastPrayerPlayed();
    unsigned long getLastPlayTimestamp();
    int getPlayedTodayMask();

    // ── Timetable offsets ───────────────────────────────────
    int getPrayerOffset(int prayerIndex);

    // ── Setters ─────────────────────────────────────────────
    void setVolume(int volume);
    void setTrackForPrayer(int prayerIndex, int track);
    void setVolumeForPrayer(int prayerIndex, int volume);
    void setLocation(float lat, float lon, const char* city, const char* country, const char* timezone);
    void setCalcMethod(const char* method);
    void setAsrJuristic(const char* juristic);
    void setHighLatitudeRule(const char* rule);
    void setTimetable(JsonObject timetable);
    void setWifi(const char* ssid, const char* password);
    void setIqamaDelay(int prayerIndex, int minutes);
    void setRecoveryState(const char* state, int prayerIndex, unsigned long timestamp);
    void setPlayedTodayMask(int mask);
    void setHijriAdjustment(int adjustment);

    // ── Config merge (for API updates) ──────────────────────
    bool mergeConfig(JsonObject incoming);

    // ── Raw document access ─────────────────────────────────
    JsonDocument& getDoc() { return _doc; }

private:
    JsonDocument _doc;
    bool _loaded = false;

    void _applyDefaults();
    void _migrateIfNeeded();

    static const char* PRAYER_KEYS[5];
};
