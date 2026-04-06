#include "ConfigManager.h"
#include "defaults.h"
#include <version.h>
#include <esp_mac.h>

const char* ConfigManager::PRAYER_KEYS[5] = {"fajr", "dhuhr", "asr", "maghrib", "isha"};

// ─────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Config] LittleFS mount failed");
        return false;
    }
    Serial.println("[Config] LittleFS mounted");

    if (LittleFS.exists(CONFIG_PATH)) {
        if (!load()) return false;
        _migrateIfNeeded();
        return true;
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

    // Validate file size to prevent OOM
    size_t fileSize = file.size();
    if (fileSize > CONFIG_MAX_SIZE) {
        Serial.printf("[Config] File too large: %u > %d\n", fileSize, CONFIG_MAX_SIZE);
        file.close();
        _applyDefaults();
        return save();
    }

    DeserializationError error = deserializeJson(_doc, file);
    file.close();

    if (error) {
        Serial.printf("[Config] JSON parse error: %s\n", error.c_str());
        _applyDefaults();
        return save();
    }

    // Validate critical fields
    if (!_validateConfig()) {
        Serial.println("[Config] Validation failed, resetting to defaults");
        _applyDefaults();
        return save();
    }

    _loaded = true;
    Serial.printf("[Config] Loaded. Device: %s, Version: %d\n", getDeviceId(), getConfigVersion());
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
    return true;
}

bool ConfigManager::reset() {
    _applyDefaults();
    return save();
}

// ─────────────────────────────────────────────────────────────
// Getters — Generic
// ─────────────────────────────────────────────────────────────

const char* ConfigManager::getDeviceId() {
    return _doc["deviceId"] | "unset";
}

int ConfigManager::getConfigVersion() {
    return _doc["configVersion"] | 1;
}

// ─────────────────────────────────────────────────────────────
// Getters — Audio
// ─────────────────────────────────────────────────────────────

int ConfigManager::getVolume() {
    return _doc["audio"]["volume"] | DEFAULT_VOLUME;
}

int ConfigManager::getTrackForPrayer(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return DEFAULT_TRACK;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["track"] | DEFAULT_TRACK;
}

int ConfigManager::getVolumeForPrayer(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return 0;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["volume"] | 0;
}

bool ConfigManager::isPrayerEnabled(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return true;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["enabled"] | true;
}

bool ConfigManager::isDoaaEnabled(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return false;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["doaa"]["enabled"] | false;
}

int ConfigManager::getDoaaTrack(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return 0;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["doaa"]["track"] | 0;
}

int ConfigManager::getDoaaDelay(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return DEFAULT_DOAA_DELAY_MIN;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["doaa"]["delayMin"] | DEFAULT_DOAA_DELAY_MIN;
}

// ─────────────────────────────────────────────────────────────
// Getters — Iqama
// ─────────────────────────────────────────────────────────────

int ConfigManager::getIqamaDelay(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return DEFAULT_IQAMA_DELAY;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["iqamaDelay"] | DEFAULT_IQAMA_DELAY;
}

int ConfigManager::getIqamaTrack(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return DEFAULT_IQAMA_TRACK;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["iqamaTrack"] | DEFAULT_IQAMA_TRACK;
}

// ─────────────────────────────────────────────────────────────
// Getters — Location / Calculation
// ─────────────────────────────────────────────────────────────

float ConfigManager::getLatitude() {
    return _doc["location"]["lat"] | 0.0f;
}

float ConfigManager::getLongitude() {
    return _doc["location"]["lon"] | 0.0f;
}

const char* ConfigManager::getTimezone() {
    return _doc["location"]["timezone"] | "UTC";
}

const char* ConfigManager::getCalcMethod() {
    return _doc["location"]["method"] | DEFAULT_CALC_METHOD;
}

const char* ConfigManager::getAsrJuristic() {
    return _doc["location"]["asrJuristic"] | DEFAULT_ASR_JURISTIC;
}

const char* ConfigManager::getHighLatitudeRule() {
    return _doc["location"]["highLatitudeRule"] | DEFAULT_HIGH_LAT_RULE;
}

// ─────────────────────────────────────────────────────────────
// Getters — Schedule
// ─────────────────────────────────────────────────────────────

bool ConfigManager::isFridayJumuahEnabled() {
    return _doc["schedule"]["fridayJumuah"] | DEFAULT_FRIDAY_JUMUAH;
}

int ConfigManager::getJumuahTrack() {
    return _doc["schedule"]["jumuahTrack"] | DEFAULT_JUMUAH_TRACK;
}

// ─────────────────────────────────────────────────────────────
// Getters — Ramadan
// ─────────────────────────────────────────────────────────────

bool ConfigManager::isRamadanModeEnabled() {
    return _doc["ramadan"]["enabled"] | true;
}

int ConfigManager::getSuhoorAlertMinutes() {
    return _doc["ramadan"]["suhoorAlertMinutes"] | DEFAULT_SUHOOR_ALERT_MIN;
}

const char* ConfigManager::getSuhoorMode() {
    return _doc["ramadan"]["suhoorMode"] | DEFAULT_SUHOOR_MODE;
}

int ConfigManager::getSuhoorTrack() {
    return _doc["ramadan"]["suhoorTrack"] | 0;
}

bool ConfigManager::isSuhoorLedEnabled() {
    return _doc["ramadan"]["suhoorLed"] | true;
}

bool ConfigManager::isIftarAlertEnabled() {
    return _doc["ramadan"]["iftarAlertEnabled"] | true;
}

int ConfigManager::getIftarTrack() {
    return _doc["ramadan"]["iftarTrack"] | 0;
}

int ConfigManager::getRamadanTrack(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return 0;
    return _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["ramadanTrack"] | 0;
}

// ─────────────────────────────────────────────────────────────
// Getters — Hijri
// ─────────────────────────────────────────────────────────────

int ConfigManager::getHijriAdjustment() {
    return _doc["hijri"]["adjustment"] | DEFAULT_HIJRI_ADJUSTMENT;
}

// ─────────────────────────────────────────────────────────────
// Getters — Holidays
// ─────────────────────────────────────────────────────────────

bool ConfigManager::isHolidayEnabled(const char* key) {
    return _doc["holidays"][key]["enabled"] | true;
}

int ConfigManager::getHolidayTrack(const char* key) {
    return _doc["holidays"][key]["track"] | 0;
}

int ConfigManager::getHolidayPostAthanTrack(const char* key) {
    return _doc["holidays"][key]["postAthanTrack"] | 0;
}

// ─────────────────────────────────────────────────────────────
// Getters — LED
// ─────────────────────────────────────────────────────────────

bool ConfigManager::isLedEnabled() {
    return _doc["led"]["enabled"] | true;
}

int ConfigManager::getPreAthanMinutes() {
    return _doc["led"]["preAthanMinutes"] | DEFAULT_PRE_ATHAN_MINUTES;
}

// ─────────────────────────────────────────────────────────────
// Getters — Multi-room
// ─────────────────────────────────────────────────────────────

const char* ConfigManager::getMultiRoomGroupId() {
    return _doc["multiRoom"]["groupId"] | "";
}

int ConfigManager::getMultiRoomSyncOffset() {
    return _doc["multiRoom"]["syncOffsetMs"] | 0;
}

// ─────────────────────────────────────────────────────────────
// Getters — Recovery
// ─────────────────────────────────────────────────────────────

const char* ConfigManager::getLastState() {
    return _doc["recovery"]["lastState"] | "idle";
}

int ConfigManager::getLastPrayerPlayed() {
    return _doc["recovery"]["lastPrayerPlayed"] | -1;
}

unsigned long ConfigManager::getLastPlayTimestamp() {
    return _doc["recovery"]["lastPlayTimestamp"] | 0UL;
}

int ConfigManager::getPlayedTodayMask() {
    return _doc["recovery"]["playedTodayMask"] | 0;
}

// ─────────────────────────────────────────────────────────────
// Getters — Timetable offsets
// ─────────────────────────────────────────────────────────────

int ConfigManager::getPrayerOffset(int idx) {
    if (idx < 0 || idx >= PRAYER_COUNT) return 0;
    return _doc["timetable"]["offsets"][PRAYER_KEYS[idx]] | 0;
}

// ─────────────────────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────────────────────

void ConfigManager::setVolume(int volume) {
    _doc["audio"]["volume"] = constrain(volume, 0, 30);
}

void ConfigManager::setTrackForPrayer(int idx, int track) {
    if (idx < 0 || idx >= PRAYER_COUNT) return;
    _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["track"] = track;
}

void ConfigManager::setVolumeForPrayer(int idx, int volume) {
    if (idx < 0 || idx >= PRAYER_COUNT) return;
    _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["volume"] = constrain(volume, 0, 30);
}

void ConfigManager::setLocation(float lat, float lon, const char* city, const char* country, const char* tz) {
    _doc["location"]["lat"] = lat;
    _doc["location"]["lon"] = lon;
    _doc["location"]["city"] = city;
    _doc["location"]["country"] = country;
    _doc["location"]["timezone"] = tz;
}

void ConfigManager::setCalcMethod(const char* method) {
    _doc["location"]["method"] = method;
}

void ConfigManager::setAsrJuristic(const char* juristic) {
    _doc["location"]["asrJuristic"] = juristic;
}

void ConfigManager::setHighLatitudeRule(const char* rule) {
    _doc["location"]["highLatitudeRule"] = rule;
}

void ConfigManager::setTimetable(JsonObject timetable) {
    _doc["timetable"] = timetable;
}

void ConfigManager::setWifi(const char* ssid, const char* password) {
    _doc["wifi"]["ssid"] = ssid;
    _doc["wifi"]["password"] = password;
}

void ConfigManager::setIqamaDelay(int idx, int minutes) {
    if (idx < 0 || idx >= PRAYER_COUNT) return;
    _doc["audio"]["prayers"][PRAYER_KEYS[idx]]["iqamaDelay"] = constrain(minutes, 0, 60);
}

void ConfigManager::setRecoveryState(const char* state, int prayerIndex, unsigned long timestamp) {
    _doc["recovery"]["lastState"] = state;
    _doc["recovery"]["lastPrayerPlayed"] = prayerIndex;
    _doc["recovery"]["lastPlayTimestamp"] = timestamp;
}

void ConfigManager::setPlayedTodayMask(int mask) {
    _doc["recovery"]["playedTodayMask"] = mask;
}

void ConfigManager::setHijriAdjustment(int adj) {
    _doc["hijri"]["adjustment"] = constrain(adj, -2, 2);
}

// ─────────────────────────────────────────────────────────────
// Config merge — partial update from API
// ─────────────────────────────────────────────────────────────

bool ConfigManager::mergeConfig(JsonObject incoming) {
    // Whitelist: only allow these top-level keys to be merged
    static const char* ALLOWED_KEYS[] = {
        "audio", "schedule", "ramadan", "hijri", "holidays",
        "led", "multiRoom", "location", "timetable", nullptr
    };

    for (JsonPair kv : incoming) {
        bool allowed = false;
        for (int i = 0; ALLOWED_KEYS[i]; i++) {
            if (strcmp(kv.key().c_str(), ALLOWED_KEYS[i]) == 0) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            Serial.printf("[Config] Rejected merge key: %s\n", kv.key().c_str());
            continue;
        }
        _doc[kv.key()] = kv.value();
    }
    return save();
}

// ─────────────────────────────────────────────────────────────
// Migration — v1 → v2
// ─────────────────────────────────────────────────────────────

void ConfigManager::_migrateIfNeeded() {
    int version = getConfigVersion();
    if (version >= CONFIG_VERSION) return;

    Serial.printf("[Config] Migrating v%d → v%d\n", version, CONFIG_VERSION);

    if (version < 2) {
        // Add new v2 fields with defaults if missing
        if (!_doc.containsKey("configVersion"))
            _doc["configVersion"] = 2;

        // Location extensions
        JsonObject loc = _doc["location"];
        if (!loc.containsKey("asrJuristic"))
            loc["asrJuristic"] = DEFAULT_ASR_JURISTIC;
        if (!loc.containsKey("highLatitudeRule"))
            loc["highLatitudeRule"] = DEFAULT_HIGH_LAT_RULE;

        // Per-prayer extensions
        for (int i = 0; i < PRAYER_COUNT; i++) {
            JsonObject p = _doc["audio"]["prayers"][PRAYER_KEYS[i]];
            if (!p.containsKey("volume"))       p["volume"] = 0;
            if (!p.containsKey("iqamaDelay"))   p["iqamaDelay"] = DEFAULT_IQAMA_DELAY;
            if (!p.containsKey("iqamaTrack"))   p["iqamaTrack"] = DEFAULT_IQAMA_TRACK;
            if (!p.containsKey("ramadanTrack")) p["ramadanTrack"] = 0;
        }

        // Schedule
        if (!_doc.containsKey("schedule")) {
            _doc["schedule"]["fridayJumuah"] = DEFAULT_FRIDAY_JUMUAH;
            _doc["schedule"]["jumuahTrack"] = DEFAULT_JUMUAH_TRACK;
        }

        // Ramadan
        if (!_doc.containsKey("ramadan")) {
            _doc["ramadan"]["enabled"] = true;
            _doc["ramadan"]["suhoorAlertMinutes"] = DEFAULT_SUHOOR_ALERT_MIN;
            _doc["ramadan"]["suhoorMode"] = DEFAULT_SUHOOR_MODE;
            _doc["ramadan"]["suhoorTrack"] = 0;
            _doc["ramadan"]["suhoorLed"] = true;
            _doc["ramadan"]["iftarAlertEnabled"] = true;
            _doc["ramadan"]["iftarTrack"] = 0;
        }

        // Hijri
        if (!_doc.containsKey("hijri")) {
            _doc["hijri"]["adjustment"] = DEFAULT_HIJRI_ADJUSTMENT;
        }

        // Holidays
        if (!_doc.containsKey("holidays")) {
            const char* holidays[] = {"eidFitr","eidAdha","mawlid","israMiraj","muharram","ashura","laylatAlQadr"};
            for (const char* h : holidays) {
                _doc["holidays"][h]["enabled"] = true;
                _doc["holidays"][h]["track"] = 0;
            }
            _doc["holidays"]["eidFitr"]["postAthanTrack"] = 0;
            _doc["holidays"]["eidAdha"]["postAthanTrack"] = 0;
        }

        // Multi-room
        if (!_doc.containsKey("multiRoom")) {
            _doc["multiRoom"]["groupId"] = "";
            _doc["multiRoom"]["syncOffsetMs"] = 0;
        }

        // Recovery
        if (!_doc.containsKey("recovery")) {
            _doc["recovery"]["lastState"] = "idle";
            _doc["recovery"]["lastPrayerPlayed"] = -1;
            _doc["recovery"]["lastPlayTimestamp"] = 0;
            _doc["recovery"]["playedTodayMask"] = 0;
        }

        // LED extensions
        JsonObject led = _doc["led"];
        if (!led.containsKey("iqamaPattern"))
            led["iqamaPattern"] = "pulse";

        _doc["configVersion"] = 2;
    }

    save();
    Serial.printf("[Config] Migration complete, now v%d\n", CONFIG_VERSION);
}

// ─────────────────────────────────────────────────────────────
// Config validation
// ─────────────────────────────────────────────────────────────

bool ConfigManager::_validateConfig() {
    // Must have deviceId
    const char* id = _doc["deviceId"] | "";
    if (strlen(id) == 0 || strlen(id) > 32) return false;

    // Volume must be 0-30
    int vol = _doc["audio"]["volume"] | -1;
    if (vol < 0 || vol > 30) {
        _doc["audio"]["volume"] = DEFAULT_VOLUME;
    }

    // Latitude -90 to 90, longitude -180 to 180
    float lat = _doc["location"]["lat"] | 0.0f;
    float lon = _doc["location"]["lon"] | 0.0f;
    if (lat < -90.0f || lat > 90.0f) _doc["location"]["lat"] = 0.0f;
    if (lon < -180.0f || lon > 180.0f) _doc["location"]["lon"] = 0.0f;

    // Hijri adjustment -2 to 2
    int adj = _doc["hijri"]["adjustment"] | 0;
    if (adj < -2 || adj > 2) _doc["hijri"]["adjustment"] = 0;

    // Per-prayer volume 0-30, iqama delay 0-60
    for (int i = 0; i < PRAYER_COUNT; i++) {
        JsonObject p = _doc["audio"]["prayers"][PRAYER_KEYS[i]];
        int pVol = p["volume"] | 0;
        if (pVol < 0 || pVol > 30) p["volume"] = 0;
        int iqama = p["iqamaDelay"] | 0;
        if (iqama < 0 || iqama > 60) p["iqamaDelay"] = 0;
    }

    // Suhoor alert 0-120 minutes
    int suhoor = _doc["ramadan"]["suhoorAlertMinutes"] | 30;
    if (suhoor < 0 || suhoor > 120) _doc["ramadan"]["suhoorAlertMinutes"] = 30;

    return true;
}

// ─────────────────────────────────────────────────────────────
// Defaults — first boot
// ─────────────────────────────────────────────────────────────

void ConfigManager::_applyDefaults() {
    _doc.clear();

    // Generate device ID from MAC
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char deviceId[18];
    snprintf(deviceId, sizeof(deviceId), "myathan-%02x%02x%02x", mac[3], mac[4], mac[5]);

    _doc["configVersion"] = CONFIG_VERSION;
    _doc["deviceId"] = deviceId;
    _doc["firmwareVersion"] = FIRMWARE_VERSION;

    _doc["wifi"]["ssid"] = "";
    _doc["wifi"]["password"] = "";

    _doc["location"]["lat"] = 0.0;
    _doc["location"]["lon"] = 0.0;
    _doc["location"]["city"] = "";
    _doc["location"]["country"] = "";
    _doc["location"]["timezone"] = "UTC";
    _doc["location"]["method"] = DEFAULT_CALC_METHOD;
    _doc["location"]["asrJuristic"] = DEFAULT_ASR_JURISTIC;
    _doc["location"]["highLatitudeRule"] = DEFAULT_HIGH_LAT_RULE;

    _doc["audio"]["volume"] = DEFAULT_VOLUME;
    _doc["audio"]["defaultTrack"] = DEFAULT_TRACK;

    for (int i = 0; i < PRAYER_COUNT; i++) {
        JsonObject p = _doc["audio"]["prayers"][PRAYER_KEYS[i]].to<JsonObject>();
        p["track"] = DEFAULT_TRACK;
        p["enabled"] = true;
        p["volume"] = 0;
        p["iqamaDelay"] = DEFAULT_IQAMA_DELAY;
        p["iqamaTrack"] = DEFAULT_IQAMA_TRACK;
        p["ramadanTrack"] = 0;
        p["doaa"]["enabled"] = false;
        p["doaa"]["track"] = 0;
        p["doaa"]["delayMin"] = DEFAULT_DOAA_DELAY_MIN;
    }

    _doc["schedule"]["fridayJumuah"] = DEFAULT_FRIDAY_JUMUAH;
    _doc["schedule"]["jumuahTrack"] = DEFAULT_JUMUAH_TRACK;

    _doc["ramadan"]["enabled"] = true;
    _doc["ramadan"]["suhoorAlertMinutes"] = DEFAULT_SUHOOR_ALERT_MIN;
    _doc["ramadan"]["suhoorMode"] = DEFAULT_SUHOOR_MODE;
    _doc["ramadan"]["suhoorTrack"] = 0;
    _doc["ramadan"]["suhoorLed"] = true;
    _doc["ramadan"]["iftarAlertEnabled"] = true;
    _doc["ramadan"]["iftarTrack"] = 0;

    _doc["hijri"]["adjustment"] = DEFAULT_HIJRI_ADJUSTMENT;

    const char* holidays[] = {"eidFitr","eidAdha","mawlid","israMiraj","muharram","ashura","laylatAlQadr"};
    for (const char* h : holidays) {
        _doc["holidays"][h]["enabled"] = true;
        _doc["holidays"][h]["track"] = 0;
    }
    _doc["holidays"]["eidFitr"]["postAthanTrack"] = 0;
    _doc["holidays"]["eidAdha"]["postAthanTrack"] = 0;

    _doc["led"]["enabled"] = true;
    _doc["led"]["preAthanMinutes"] = DEFAULT_PRE_ATHAN_MINUTES;
    _doc["led"]["preAthanPattern"] = "slow_blink";
    _doc["led"]["playingPattern"] = "solid";
    _doc["led"]["iqamaPattern"] = "pulse";
    _doc["led"]["errorPattern"] = "fast_blink";
    _doc["led"]["noWifiPattern"] = "pulse";

    _doc["multiRoom"]["groupId"] = "";
    _doc["multiRoom"]["syncOffsetMs"] = 0;

    _doc["recovery"]["lastState"] = "idle";
    _doc["recovery"]["lastPrayerPlayed"] = -1;
    _doc["recovery"]["lastPlayTimestamp"] = 0;
    _doc["recovery"]["playedTodayMask"] = 0;

    _doc["timetable"]["fetchedAt"] = "";
    for (int i = 0; i < PRAYER_COUNT; i++) {
        _doc["timetable"]["offsets"][PRAYER_KEYS[i]] = 0;
    }

    _doc["ota"]["checkHour"] = DEFAULT_OTA_CHECK_HOUR;
    _doc["ota"]["lastChecked"] = "";

    _doc["stats"]["lastSent"] = "";

    _loaded = true;
}
