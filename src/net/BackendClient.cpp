#include "BackendClient.h"
#include <myathan_version.h>
#include "../config/ConfigManager.h"
#include "../config/defaults.h"
#include "../time/NtpSync.h"
#include "../sync/MultiRoomSync.h"
#include "../ota/OtaManager.h"
#include "../prayer/PrayerScheduler.h"
#include "../wifi/WifiProvisioner.h"
#include <HTTPClient.h>
#include <WiFi.h>

void BackendClient::begin(ConfigManager* config, NtpSync* ntp, MultiRoomSync* sync,
                          OtaManager* ota, PrayerScheduler* scheduler, WifiProvisioner* wifi) {
    _config = config;
    _ntp = ntp;
    _sync = sync;
    _ota = ota;
    _scheduler = scheduler;
    _wifi = wifi;

    _baseUrl = "https://api.myathan.com";  // TODO: make configurable
    Serial.println("[Backend] Client initialized");
}

void BackendClient::update() {
    if (!WiFi.isConnected() || !_ntp || !_ntp->isSynced()) return;

    unsigned long now = millis();

    // Register on first successful connection
    if (!_registered) {
        registerDevice();
        return;
    }

    // Config poll every 5 minutes
    if (now - _lastConfigPoll >= DEFAULT_CONFIG_POLL_INTERVAL_MS) {
        fetchConfig();
        _lastConfigPoll = now;
    }

    // Heartbeat every hour
    if (now - _lastHeartbeat >= DEFAULT_HEARTBEAT_INTERVAL_MS) {
        sendHeartbeat();
        _lastHeartbeat = now;
    }

    // OTA check based on user preferences
    if (_config && _ntp && _ntp->isSynced()) {
        String otaFreq = _config->getOtaCheckFrequency();
        if (otaFreq != "manual") {
            int checkHour = _config->getOtaCheckHour();
            struct tm timeinfo;
            getLocalTime(&timeinfo);

            bool shouldCheck = false;
            if (otaFreq == "daily" && timeinfo.tm_hour == checkHour) {
                shouldCheck = (now - _lastOtaCheck > 3600000); // max once per hour
            } else if (otaFreq == "weekly" && timeinfo.tm_wday == 0 && timeinfo.tm_hour == checkHour) {
                shouldCheck = (now - _lastOtaCheck > 3600000);
            }

            if (shouldCheck) {
                checkOtaUpdate();
                _lastOtaCheck = now;
            }
        }
    }
}

bool BackendClient::registerDevice() {
    JsonDocument body;
    body["deviceId"] = _config->getDeviceId();
    body["firmwareVersion"] = FIRMWARE_VERSION;

    JsonDocument response;
    if (!_httpPost("/api/device/register", body, response)) {
        return false;
    }

    _apiKey = response["apiKey"].as<String>();
    _registered = true;
    Serial.printf("[Backend] Registered, API key received\n");
    return true;
}

bool BackendClient::fetchConfig() {
    if (!hasApiKey()) return false;

    JsonDocument response;
    if (!_httpGet("/api/device/config", response)) {
        return false;
    }

    JsonObject config = response["config"];
    if (config.size() > 0) {
        _config->mergeConfig(config);
        Serial.println("[Backend] Config updated from server");
    }

    return true;
}

bool BackendClient::sendHeartbeat() {
    if (!hasApiKey()) return false;

    JsonDocument body;
    body["firmwareVersion"] = FIRMWARE_VERSION;
    body["hardwareType"] = HARDWARE_TYPE;
    body["freeHeap"] = ESP.getFreeHeap();
    body["wifiRssi"] = WiFi.RSSI();
    body["uptime"] = millis() / 1000;
    body["errors"] = _errorCount;

    if (_ota && _ota->justUpdated()) {
        body["justUpdated"] = true;
    }

    // Location
    body["lat"] = _config->getLatitude();
    body["lon"] = _config->getLongitude();

    // Prayer plays from scheduler bitmask
    if (_scheduler) {
        JsonObject plays = body["prayerPlays"].to<JsonObject>();
        int mask = _scheduler->getPlayedTodayMask();
        const char* names[] = {"fajr", "dhuhr", "asr", "maghrib", "isha"};
        for (int i = 0; i < 5; i++) {
            plays[names[i]] = (mask & (1 << i)) ? 1 : 0;
        }
    }

    JsonDocument response;
    if (!_httpPost("/api/device/heartbeat", body, response)) {
        _errorCount++;
        return false;
    }
    _errorCount = 0;

    // Check for sync triggers
    if (response.containsKey("syncTrigger") && _sync) {
        int prayer = response["syncTrigger"]["prayer"] | -1;
        unsigned long epoch = response["syncTrigger"]["triggerAtEpoch"] | 0UL;
        if (prayer >= 0 && epoch > 0) {
            _sync->setSyncTrigger(prayer, epoch);
        }
    }

    // Check for pending commands
    if (response.containsKey("command")) {
        const char* cmd = response["command"]["command"] | "";
        if (strlen(cmd) == 0) return true;
        Serial.printf("[Backend] Command received: %s\n", cmd);

        if (strcmp(cmd, "restart") == 0) {
            Serial.println("[Backend] Restarting in 3s...");
            delay(3000);
            ESP.restart();
        } else if (strcmp(cmd, "wifi_reset") == 0 && _wifi) {
            Serial.println("[Backend] Resetting WiFi credentials");
            _wifi->resetCredentials();
        } else if (strcmp(cmd, "ota_update") == 0 && _ota) {
            OtaUpdateInfo info = {};
            strlcpy(info.version, response["command"]["payload"]["version"] | "", sizeof(info.version));
            strlcpy(info.url, response["command"]["payload"]["url"] | "", sizeof(info.url));
            info.size = response["command"]["payload"]["size"] | 0;
            strlcpy(info.sha256, response["command"]["payload"]["sha256"] | "", sizeof(info.sha256));
            if (strlen(info.url) > 0) {
                _ota->applyUpdate(info);
            }
        }
    }

    Serial.println("[Backend] Heartbeat sent");
    return true;
}

bool BackendClient::fetchTimetable() {
    char path[128];
    snprintf(path, sizeof(path),
        "/api/device/timetable?lat=%.4f&lon=%.4f&method=%s&asr=%s",
        _config->getLatitude(), _config->getLongitude(),
        _config->getCalcMethod(), _config->getAsrJuristic());

    JsonDocument response;
    if (!_httpGet(path, response)) {
        return false;
    }

    Serial.println("[Backend] Timetable fetched from server");
    return true;
}

bool BackendClient::checkOtaUpdate() {
    if (!hasApiKey()) return false;

    char path[128];
    snprintf(path, sizeof(path),
        "/api/device/ota/check?currentVersion=%s&hardwareType=%s",
        FIRMWARE_VERSION, HARDWARE_TYPE);

    JsonDocument response;
    if (!_httpGet(path, response)) {
        return false;
    }

    bool available = response["updateAvailable"] | false;
    if (available && _ota) {
        OtaUpdateInfo info = {};
        strlcpy(info.version, response["version"] | "", sizeof(info.version));
        strlcpy(info.sha256, response["sha256"] | "", sizeof(info.sha256));
        strlcpy(info.url, response["url"] | "", sizeof(info.url));
        info.size = response["size"] | 0;

        Serial.printf("[Backend] OTA update available: v%s (%d bytes)\n", info.version, info.size);

        if (strlen(info.url) > 0 && info.size > 0) {
            _ota->applyUpdate(info);
            // If we get here, update failed (applyUpdate reboots on success)
        }
    }

    return true;
}

// ─────────────────────────────────────────────────────────────
// HTTP Helpers
// ─────────────────────────────────────────────────────────────

bool BackendClient::_httpGet(const char* path, JsonDocument& response) {
    HTTPClient http;
    String url = _baseUrl + path;

    http.begin(url);
    if (hasApiKey()) {
        http.addHeader("Authorization", "Bearer " + _apiKey);
    }

    int code = http.GET();
    if (code < 200 || code >= 300) {
        snprintf(_lastError, sizeof(_lastError), "GET %s → %d", path, code);
        Serial.printf("[Backend] %s\n", _lastError);
        http.end();
        return false;
    }

    DeserializationError err = deserializeJson(response, http.getStream());
    http.end();

    if (err) {
        snprintf(_lastError, sizeof(_lastError), "JSON parse: %s", err.c_str());
        return false;
    }

    return true;
}

bool BackendClient::_httpPost(const char* path, const JsonDocument& body, JsonDocument& response) {
    HTTPClient http;
    String url = _baseUrl + path;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    if (hasApiKey()) {
        http.addHeader("Authorization", "Bearer " + _apiKey);
    }

    String bodyStr;
    serializeJson(body, bodyStr);

    int code = http.POST(bodyStr);
    if (code < 200 || code >= 300) {
        snprintf(_lastError, sizeof(_lastError), "POST %s → %d", path, code);
        Serial.printf("[Backend] %s\n", _lastError);
        http.end();
        return false;
    }

    DeserializationError err = deserializeJson(response, http.getStream());
    http.end();

    if (err) {
        snprintf(_lastError, sizeof(_lastError), "JSON parse: %s", err.c_str());
        return false;
    }

    return true;
}

bool BackendClient::_httpPut(const char* path, const JsonDocument& body, JsonDocument& response) {
    HTTPClient http;
    String url = _baseUrl + path;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    if (hasApiKey()) {
        http.addHeader("Authorization", "Bearer " + _apiKey);
    }

    String bodyStr;
    serializeJson(body, bodyStr);

    int code = http.PUT(bodyStr);
    if (code < 200 || code >= 300) {
        snprintf(_lastError, sizeof(_lastError), "PUT %s → %d", path, code);
        http.end();
        return false;
    }

    DeserializationError err = deserializeJson(response, http.getStream());
    http.end();

    return !err;
}
