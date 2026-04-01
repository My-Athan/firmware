#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

#define CONFIG_PATH "/config.json"
#define CONFIG_MAX_SIZE 4096

class ConfigManager {
public:
    bool begin();
    bool load();
    bool save();
    bool reset();

    // Getters
    const char* getDeviceId();
    int getVolume();
    int getTrackForPrayer(int prayerIndex);
    bool isDoaaEnabled(int prayerIndex);
    int getDoaaTrack(int prayerIndex);
    int getDoaaDelay(int prayerIndex);
    int getPreAthanMinutes();
    const char* getPrayerTime(int prayerIndex);

    // Setters
    void setVolume(int volume);
    void setTrackForPrayer(int prayerIndex, int track);
    void setLocation(float lat, float lon, const char* city, const char* timezone);
    void setTimetable(JsonObject timetable);
    void setWifi(const char* ssid, const char* password);

    // Raw document access (for API responses)
    JsonDocument& getDoc() { return _doc; }

private:
    JsonDocument _doc;
    bool _loaded = false;

    void _applyDefaults();
};
