#include <Arduino.h>
#include "version.h"

// ── Managers ────────────────────────────────────────────────
#include "config/ConfigManager.h"
#include "wifi/WifiProvisioner.h"
#include "time/NtpSync.h"
#include "audio/AudioManager.h"
#include "led/LedManager.h"
#include "input/ButtonHandler.h"
#include "prayer/PrayerScheduler.h"
#include "prayer/IqamaTimer.h"
#include "api/LocalServer.h"
#include "sync/MultiRoomSync.h"
#include "net/BackendClient.h"
#include "net/OfflineCache.h"
#include "ota/OtaManager.h"

// ── Global instances ────────────────────────────────────────
ConfigManager   configManager;
WifiProvisioner wifi;
NtpSync         ntp;
AudioManager    audio;
LedManager      led;
ButtonHandler   button;
PrayerScheduler scheduler;
IqamaTimer      iqama;
LocalServer     server;
MultiRoomSync   multiRoom;
BackendClient   backend;
OfflineCache    cache;
OtaManager      ota;

// ── Callbacks ───────────────────────────────────────────────
void onWifiConnected() {
    Serial.println("[Main] WiFi connected — starting NTP sync");
    ntp.begin(configManager.getTimezone());
    led.setState(LedState::IDLE);

    // Start local HTTP server
    server.begin(&configManager, &audio, &scheduler, &ntp);
}

void onWifiCredentials(const char* ssid, const char* password) {
    configManager.setWifi(ssid, password);
    configManager.save();
    Serial.printf("[Main] WiFi credentials saved: %s\n", ssid);
}

// ─────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("================================");
    Serial.printf("MyAthan Firmware v%s\n", FIRMWARE_VERSION);
    Serial.printf("Build: %s\n", FIRMWARE_BUILD);
    Serial.println("================================");

    // 1. Config — must be first (all others depend on it)
    if (!configManager.begin()) {
        Serial.println("[FATAL] Config init failed!");
        led.begin();
        led.setState(LedState::ERROR);
        return;
    }

    // 2. LED — early init for visual feedback
    led.begin();
    led.setState(LedState::PROVISIONING);

    // 3. Button
    button.begin();

    // 4. Audio (DFPlayer Mini)
    if (!audio.begin()) {
        Serial.println("[WARN] Audio init failed — check DFPlayer/SD card");
        led.setState(LedState::ERROR);
    }

    // 5. Prayer engine (calculator, hijri, iqama)
    iqama.begin(&configManager, &audio, &led);
    scheduler.begin(&configManager, &audio, &led, &ntp, &iqama);

    // 6. Multi-room sync
    multiRoom.begin(&configManager, &audio, &ntp, &scheduler);

    // 7. OTA manager + backend client + offline cache
    ota.begin(&configManager, &ntp, &led);
    cache.begin();
    backend.begin(&configManager, &ntp, &multiRoom, &ota);

    // 8. WiFi — last in setup, may block for provisioning
    wifi.onConnected(onWifiConnected);
    wifi.onCredentials(onWifiCredentials);

    const char* ssid = configManager.getDoc()["wifi"]["ssid"] | "";
    const char* pass = configManager.getDoc()["wifi"]["password"] | "";
    wifi.begin(ssid, pass);

    if (!wifi.isConnected()) {
        led.setState(LedState::NO_WIFI);
    }

    Serial.println("[Main] Setup complete");
}

// ─────────────────────────────────────────────────────────────
// Loop
// ─────────────────────────────────────────────────────────────

void loop() {
    // WiFi state management
    wifi.update();

    // NTP re-sync
    if (wifi.isConnected()) {
        ntp.update();
    }

    // Prayer scheduler (calculates times, triggers athan)
    scheduler.tick();

    // Iqama countdown
    iqama.update();

    // Multi-room sync check
    multiRoom.update();

    // Backend polling (config sync, heartbeat, OTA check)
    backend.update();

    // Audio state (preview timeout, playback detection)
    audio.update();

    // LED patterns
    led.update();

    // Button input
    ButtonEvent evt = button.update();
    switch (evt) {
        case ButtonEvent::SHORT_PRESS:
            // Trigger next prayer's athan
            {
                int next = scheduler.getNextPrayerIndex();
                if (next >= 0) scheduler.triggerAthan(next);
            }
            break;

        case ButtonEvent::DOUBLE_PRESS:
            // Preview current athan track
            {
                int next = scheduler.getNextPrayerIndex();
                if (next >= 0) {
                    int track = configManager.getTrackForPrayer(next);
                    scheduler.triggerPreview(track);
                }
            }
            break;

        case ButtonEvent::LONG_PRESS:
            // Reset WiFi credentials
            led.setState(LedState::PROVISIONING);
            wifi.resetCredentials();
            break;

        default:
            break;
    }

    // Small yield to prevent watchdog
    delay(10);
}
