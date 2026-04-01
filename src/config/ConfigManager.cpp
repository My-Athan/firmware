#include "ConfigManager.h"
#include "defaults.h"
#include <esp_mac.h>

static const char* PRAYER_KEYS[] = {"fajr", "dhuhr", "asr", "maghrib", "isha"};

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Config] LittleFS mount failed");
        return false;
    }
    Serial.println("[Config] LittleFS mounted");

    if (LittleFS.exists(CONFIG_PATH)) {
        return load();
    } else {
        Serial.println("[Config] No config found, applying defaults");
        _applyDefaults();
        return save();
    }
}

bool ConfigManager::load() {
    File file = LittleFS.open(CONFIG_PATH, "r");
    if (!file) {
        Serial.println("[Config] Failed to open config file");
        return false;
    }

    DeserializationError error = deserializeJson(_doc, file);
    file.close();

    if (error) {
        Serial.printf("[Config] JSON parse error: %s\n", error.c_str());
        _applyDefaults();
        return save();
    }

    _loaded = true;
    Serial.printf("[Config] Loaded. Device ID: %s\n", getDeviceId());
    return true;
}

bool ConfigManager::save() {
    File file = LittleFS.open(CONFIG_PATH, "w");
    if (!file) {
        Serial.println("[Config] Failed to open config for writing");
        return false;
    }

    serializeJsonPretty(_doc, file);
    file.close();
    Serial.println("[Config] Saved");
    return true;
}

bool ConfigManager::reset() {
    _applyDefaults();
    return save();
}

const char* ConfigManager::getDeviceId() {
    return _doc["deviceId"] | "unset";
}

int ConfigManager::getVolume() {
    return _doc["audio"]["volume"] | DEFAULT_VOLUME;
}

int ConfigManager::getTrackForPrayer(int prayerIndex) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return DEFAULT_TRACK;
    return _doc["audio"]["prayers"][PRAYER_KEYS[prayerIndex]]["track"] | DEFAULT_TRACK;
}

bool ConfigManager::isDoaaEnabled(int prayerIndex) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return false;
    return _doc["audio"]["prayers"][PRAYER_KEYS[prayerIndex]]["doaa"]["enabled"] | false;
}

int ConfigManager::getDoaaTrack(int prayerIndex) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return 0;
    return _doc["audio"]["prayers"][PRAYER_KEYS[prayerIndex]]["doaa"]["track"] | 0;
}

int ConfigManager::getDoaaDelay(int prayerIndex) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return 3;
    return _doc["audio"]["prayers"][PRAYER_KEYS[prayerIndex]]["doaa"]["delayMin"] | 3;
}

int ConfigManager::getPreAthanMinutes() {
    return _doc["led"]["preAthanMinutes"] | DEFAULT_PRE_ATHAN_MINUTES;
}

const char* ConfigManager::getPrayerTime(int prayerIndex) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return "00:00";
    // Get today's timetable — caller should check date freshness
    JsonObject days = _doc["timetable"]["days"];
    if (days.size() == 0) return "00:00";
    // Return first available day's prayer time
    JsonObject firstDay = days.begin()->value();
    return firstDay[PRAYER_KEYS[prayerIndex]] | "00:00";
}

void ConfigManager::setVolume(int volume) {
    _doc["audio"]["volume"] = constrain(volume, 0, 30);
}

void ConfigManager::setTrackForPrayer(int prayerIndex, int track) {
    if (prayerIndex < 0 || prayerIndex >= PRAYER_COUNT) return;
    _doc["audio"]["prayers"][PRAYER_KEYS[prayerIndex]]["track"] = track;
}

void ConfigManager::setLocation(float lat, float lon, const char* city, const char* timezone) {
    _doc["location"]["lat"] = lat;
    _doc["location"]["lon"] = lon;
    _doc["location"]["city"] = city;
    _doc["location"]["timezone"] = timezone;
}

void ConfigManager::setTimetable(JsonObject timetable) {
    _doc["timetable"] = timetable;
}

void ConfigManager::setWifi(const char* ssid, const char* password) {
    _doc["wifi"]["ssid"] = ssid;
    _doc["wifi"]["password"] = password;
}

void ConfigManager::_applyDefaults() {
    _doc.clear();

    // Generate a simple device ID from MAC
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char deviceId[18];
    snprintf(deviceId, sizeof(deviceId), "myathan-%02x%02x%02x",
             mac[3], mac[4], mac[5]);

    _doc["deviceId"] = deviceId;
    _doc["firmwareVersion"] = FIRMWARE_VERSION;

    _doc["wifi"]["ssid"] = "";
    _doc["wifi"]["password"] = "";

    _doc["location"]["lat"] = 0.0;
    _doc["location"]["lon"] = 0.0;
    _doc["location"]["city"] = "";
    _doc["location"]["country"] = "";
    _doc["location"]["timezone"] = "UTC";
    _doc["location"]["method"] = "ISNA";

    _doc["audio"]["volume"] = DEFAULT_VOLUME;
    _doc["audio"]["defaultTrack"] = DEFAULT_TRACK;

    for (int i = 0; i < PRAYER_COUNT; i++) {
        _doc["audio"]["prayers"][PRAYER_KEYS[i]]["track"] = DEFAULT_TRACK;
        _doc["audio"]["prayers"][PRAYER_KEYS[i]]["enabled"] = true;
        _doc["audio"]["prayers"][PRAYER_KEYS[i]]["doaa"]["enabled"] = false;
    }

    _doc["led"]["enabled"] = true;
    _doc["led"]["preAthanMinutes"] = DEFAULT_PRE_ATHAN_MINUTES;
    _doc["led"]["preAthanPattern"] = "slow_blink";
    _doc["led"]["playingPattern"] = "solid";
    _doc["led"]["errorPattern"] = "fast_blink";
    _doc["led"]["noWifiPattern"] = "pulse";

    _doc["ota"]["checkHour"] = DEFAULT_OTA_CHECK_HOUR;
    _doc["ota"]["lastChecked"] = "";

    _doc["stats"]["lastSent"] = "";

    _loaded = true;
}
