#include <Arduino.h>
#include "version.h"
#include "config/ConfigManager.h"

ConfigManager configManager;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("================================");
    Serial.printf("MyAthan Firmware v%s\n", FIRMWARE_VERSION);
    Serial.printf("Build: %s\n", FIRMWARE_BUILD);
    Serial.println("================================");

    // Phase 1: Initialize config from LittleFS
    if (!configManager.begin()) {
        Serial.println("[ERROR] Failed to initialize config");
    }

    // TODO Phase 1: WiFi provisioning (BLE + WiFiManager fallback)
    // TODO Phase 1: NTP time sync
    // TODO Phase 1: DFPlayer Mini audio driver
    // TODO Phase 1: LED state machine
    // TODO Phase 1: Prayer scheduler
    // TODO Phase 1: Button handler
    // TODO Phase 1: Local HTTP server (ESPAsyncWebServer)
}

void loop() {
    // TODO Phase 1: Prayer scheduler tick (check every 30s)
    // TODO Phase 1: LED pattern update
    // TODO Phase 2: Config polling (every 5 min)
    // TODO Phase 2: Heartbeat (every hour)
    delay(1000);
}
