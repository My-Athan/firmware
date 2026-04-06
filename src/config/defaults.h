#pragma once

// ──────────────────────────────────────────────────────────────
// MyAthan — Default configuration values
// Used when no config.json exists on LittleFS (first boot)
// ──────────────────────────────────────────────────────────────

// Config schema version — bump when config.json format changes
#define CONFIG_VERSION 2

// ── Audio ────────────────────────────────────────────────────
#define DEFAULT_VOLUME 20          // 0-30, DFPlayer range
#define DEFAULT_TRACK 1            // SD card track number
#define DEFAULT_DOAA_DELAY_MIN 3   // Minutes after athan before doaa

// ── LED / Pre-athan ─────────────────────────────────────────
#define DEFAULT_PRE_ATHAN_MINUTES 10

// ── Cloud / Polling ─────────────────────────────────────────
#define DEFAULT_OTA_CHECK_HOUR 3                    // 3 AM
#define DEFAULT_HEARTBEAT_INTERVAL_MS 3600000       // 1 hour
#define DEFAULT_CONFIG_POLL_INTERVAL_MS 300000       // 5 minutes
#define DEFAULT_STATS_INTERVAL_MS 86400000           // 24 hours

// ── Pin assignments — ESP32-C3 SuperMini ────────────────────
// UART0 (Serial/CDC) → logging
// UART1 (Hardware)   → DFPlayer Mini
#define PIN_DFPLAYER_TX 6
#define PIN_DFPLAYER_RX 7
#define PIN_LED 8
#define PIN_BUTTON 9

// ── DFPlayer ────────────────────────────────────────────────
#define DFPLAYER_BAUD 9600
#define DFPLAYER_UART 1  // Hardware Serial1

// ── Prayer indices ──────────────────────────────────────────
#define PRAYER_FAJR     0
#define PRAYER_DHUHR    1
#define PRAYER_ASR      2
#define PRAYER_MAGHRIB  3
#define PRAYER_ISHA     4
#define PRAYER_COUNT    5

// ── Prayer calculation ──────────────────────────────────────
#define DEFAULT_CALC_METHOD       "ISNA"
#define DEFAULT_ASR_JURISTIC      "standard"     // "standard" or "hanafi"
#define DEFAULT_HIGH_LAT_RULE     "angle_based"  // "angle_based", "midnight", "one_seventh", "none"

// ── Iqama ───────────────────────────────────────────────────
#define DEFAULT_IQAMA_DELAY       0   // 0 = disabled, 1-60 minutes
#define DEFAULT_IQAMA_TRACK       0   // 0 = use default beep

// ── Ramadan / Suhoor ────────────────────────────────────────
#define DEFAULT_SUHOOR_ALERT_MIN  30  // Minutes before Fajr
#define DEFAULT_SUHOOR_MODE       "sound"  // "none", "sound", "led", "custom"

// ── Hijri ───────────────────────────────────────────────────
#define DEFAULT_HIJRI_ADJUSTMENT  0   // -2 to +2 days

// ── Schedule ────────────────────────────────────────────────
#define DEFAULT_FRIDAY_JUMUAH     true
#define DEFAULT_JUMUAH_TRACK      1

// ── Multi-room ──────────────────────────────────────────────
#define DEFAULT_MULTIROOM_GROUP   ""  // Empty = standalone

// ── Button ──────────────────────────────────────────────────
#define BUTTON_DEBOUNCE_MS        50
#define BUTTON_LONG_PRESS_MS      5000
#define BUTTON_DOUBLE_PRESS_MS    400

// ── NTP ─────────────────────────────────────────────────────
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"
#define NTP_SYNC_INTERVAL_MS 3600000  // Re-sync every hour

// ── WiFi provisioning ───────────────────────────────────────
#define BLE_ADVERTISING_TIMEOUT_MS 60000  // 60s BLE window, then WiFiManager
#define WIFI_CONNECT_TIMEOUT_MS    30000  // 30s WiFi connection attempt

// ── Preview ─────────────────────────────────────────────────
#define PREVIEW_DURATION_MS 10000  // 10s preview playback
