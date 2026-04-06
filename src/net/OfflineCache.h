#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// ─────────────────────────────────────────────────────────────
// Offline Cache — stores server timetable for 7 days
// Falls back to on-device PrayerCalculator when offline.
// ─────────────────────────────────────────────────────────────

#define CACHE_PATH "/cache"
#define CACHE_MAX_DAYS 7

class OfflineCache {
public:
    bool begin();

    // Store a day's timetable from the server
    bool cacheDay(const char* dateKey, JsonObject timetable);

    // Retrieve a cached day (returns false if not cached)
    bool getDay(const char* dateKey, JsonDocument& out);

    // Check if a specific day is cached
    bool hasDay(const char* dateKey);

    // Clean entries older than CACHE_MAX_DAYS
    void cleanup();

    // Get cache stats
    int cachedDayCount();

private:
    String _pathForDate(const char* dateKey);
};
