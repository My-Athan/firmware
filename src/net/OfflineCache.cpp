#include "OfflineCache.h"

bool OfflineCache::begin() {
    if (!LittleFS.exists(CACHE_PATH)) {
        LittleFS.mkdir(CACHE_PATH);
    }
    Serial.printf("[Cache] Initialized, %d days cached\n", cachedDayCount());
    return true;
}

bool OfflineCache::cacheDay(const char* dateKey, JsonObject timetable) {
    String path = _pathForDate(dateKey);

    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.printf("[Cache] Failed to write %s\n", path.c_str());
        return false;
    }

    serializeJson(timetable, file);
    file.close();

    Serial.printf("[Cache] Cached timetable for %s\n", dateKey);
    cleanup();
    return true;
}

bool OfflineCache::getDay(const char* dateKey, JsonDocument& out) {
    String path = _pathForDate(dateKey);

    if (!LittleFS.exists(path)) return false;

    File file = LittleFS.open(path, "r");
    if (!file) return false;

    DeserializationError err = deserializeJson(out, file);
    file.close();

    return !err;
}

bool OfflineCache::hasDay(const char* dateKey) {
    return LittleFS.exists(_pathForDate(dateKey));
}

void OfflineCache::cleanup() {
    File dir = LittleFS.open(CACHE_PATH);
    if (!dir || !dir.isDirectory()) return;

    // Count files and delete oldest if over limit
    int count = 0;
    String oldest;
    File file = dir.openNextFile();

    while (file) {
        count++;
        String name = file.name();
        if (oldest.isEmpty() || name < oldest) {
            oldest = name;
        }
        file = dir.openNextFile();
    }

    while (count > CACHE_MAX_DAYS) {
        if (oldest.length() > 0) {
            String path = String(CACHE_PATH) + "/" + oldest;
            LittleFS.remove(path);
            Serial.printf("[Cache] Removed old entry: %s\n", oldest.c_str());
        }
        count--;

        // Re-find oldest
        oldest = "";
        dir.rewindDirectory();
        file = dir.openNextFile();
        while (file) {
            String name = file.name();
            if (oldest.isEmpty() || name < oldest) {
                oldest = name;
            }
            file = dir.openNextFile();
        }
    }
}

int OfflineCache::cachedDayCount() {
    File dir = LittleFS.open(CACHE_PATH);
    if (!dir || !dir.isDirectory()) return 0;

    int count = 0;
    File file = dir.openNextFile();
    while (file) {
        count++;
        file = dir.openNextFile();
    }
    return count;
}

String OfflineCache::_pathForDate(const char* dateKey) {
    // Validate dateKey format: must be YYYY-MM-DD (10 chars)
    if (!dateKey || strlen(dateKey) != 10) {
        return String(CACHE_PATH) + "/invalid.json";
    }
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (dateKey[i] != '-') return String(CACHE_PATH) + "/invalid.json";
        } else {
            if (dateKey[i] < '0' || dateKey[i] > '9') return String(CACHE_PATH) + "/invalid.json";
        }
    }
    return String(CACHE_PATH) + "/" + dateKey + ".json";
}
