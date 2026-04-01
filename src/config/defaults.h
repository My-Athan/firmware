#pragma once

// Default configuration values for MyAthan device
// These are used when no config.json exists on LittleFS (first boot)

#define DEFAULT_VOLUME 20
#define DEFAULT_TRACK 1
#define DEFAULT_PRE_ATHAN_MINUTES 10
#define DEFAULT_OTA_CHECK_HOUR 3
#define DEFAULT_HEARTBEAT_INTERVAL_MS 3600000  // 1 hour
#define DEFAULT_CONFIG_POLL_INTERVAL_MS 300000  // 5 minutes
#define DEFAULT_STATS_INTERVAL_MS 86400000      // 24 hours

// Pin assignments — ESP32-C3 SuperMini
#define PIN_DFPLAYER_TX 4
#define PIN_DFPLAYER_RX 5
#define PIN_LED 8
#define PIN_BUTTON 9

// DFPlayer
#define DFPLAYER_BAUD 9600

// Prayer names (index into config arrays)
#define PRAYER_FAJR 0
#define PRAYER_DHUHR 1
#define PRAYER_ASR 2
#define PRAYER_MAGHRIB 3
#define PRAYER_ISHA 4
#define PRAYER_COUNT 5
