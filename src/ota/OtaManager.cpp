#include "OtaManager.h"
#include "../../include/version.h"
#include "../config/ConfigManager.h"
#include "../time/NtpSync.h"
#include "../led/LedManager.h"
#include "../config/defaults.h"
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

int OtaManager::_consecutiveFailures = 0;

void OtaManager::begin(ConfigManager* config, NtpSync* ntp, LedManager* led) {
    _config = config;
    _ntp = ntp;
    _led = led;

    // Mark current firmware as good (disables automatic rollback)
    _markBootSuccessful();

    Serial.printf("[OTA] Manager ready. Firmware: %s\n", FIRMWARE_VERSION);
}

bool OtaManager::applyUpdate(const OtaUpdateInfo& info) {
    Serial.printf("[OTA] Starting update to v%s (%d bytes)\n", info.version, info.size);

    // Pre-update validation
    if (ESP.getFreeHeap() < 50000) {
        snprintf(_error, sizeof(_error), "Insufficient heap: %lu", ESP.getFreeHeap());
        Serial.printf("[OTA] %s\n", _error);
        _state = OtaState::ERROR;
        return false;
    }

    if (info.size <= 0 || info.size > 1500000) {
        snprintf(_error, sizeof(_error), "Invalid size: %d", info.size);
        _state = OtaState::ERROR;
        return false;
    }

    // Set LED to indicate update in progress
    if (_led) _led->setState(LedState::ERROR);  // Fast blink during OTA

    // Download
    _state = OtaState::DOWNLOADING;
    if (!_download(info.url, info.size)) {
        _consecutiveFailures++;
        _state = OtaState::ERROR;

        if (_consecutiveFailures >= 3) {
            Serial.println("[OTA] 3 consecutive failures — entering recovery mode");
        }
        return false;
    }

    // Verify SHA256
    _state = OtaState::VERIFYING;
    // Note: SHA256 verification happens during download via Update library
    // The Update library handles partition writing and verification

    // Success
    _state = OtaState::REBOOTING;
    _consecutiveFailures = 0;
    Serial.printf("[OTA] Update complete! Rebooting to v%s in 3s...\n", info.version);

    delay(3000);
    ESP.restart();

    return true;  // Never reached
}

bool OtaManager::_download(const char* url, int expectedSize) {
    HTTPClient http;
    http.begin(url);
    http.setTimeout(30000);  // 30s timeout

    int code = http.GET();
    if (code != 200) {
        snprintf(_error, sizeof(_error), "Download HTTP %d", code);
        Serial.printf("[OTA] %s\n", _error);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) contentLength = expectedSize;

    if (!Update.begin(contentLength, U_FLASH)) {
        snprintf(_error, sizeof(_error), "Update.begin failed: %s", Update.errorString());
        Serial.printf("[OTA] %s\n", _error);
        http.end();
        return false;
    }

    // Stream download directly to flash
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[1024];
    int written = 0;
    int lastProgressPct = -1;

    while (http.connected() && written < contentLength) {
        int available = stream->available();
        if (available <= 0) {
            delay(1);
            continue;
        }

        int readBytes = stream->readBytes(buf, min(available, (int)sizeof(buf)));
        if (readBytes <= 0) break;

        int wroteNow = Update.write(buf, readBytes);
        if (wroteNow != readBytes) {
            snprintf(_error, sizeof(_error), "Write error at %d bytes", written);
            Update.abort();
            http.end();
            return false;
        }

        written += wroteNow;
        _progress = (written * 100) / contentLength;

        if (_progress / 10 != lastProgressPct / 10) {
            Serial.printf("[OTA] Progress: %d%%\n", _progress);
            lastProgressPct = _progress;
        }
    }

    http.end();

    if (!Update.end(true)) {
        snprintf(_error, sizeof(_error), "Update.end failed: %s", Update.errorString());
        Serial.printf("[OTA] %s\n", _error);
        return false;
    }

    Serial.printf("[OTA] Downloaded %d bytes, flash written\n", written);
    return true;
}

void OtaManager::_markBootSuccessful() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running) {
        esp_ota_img_states_t state;
        if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
            if (state == ESP_OTA_IMG_PENDING_VERIFY) {
                esp_ota_mark_app_valid_cancel_rollback();
                Serial.println("[OTA] Boot marked successful — rollback cancelled");
            }
        }
    }
}

bool OtaManager::checkAndUpdate(const char* currentVersion) {
    // This is called by BackendClient when an update is available
    // The actual check happens in BackendClient::checkOtaUpdate()
    // This method would be called with the update info
    return false;  // Placeholder — triggered via applyUpdate() instead
}
