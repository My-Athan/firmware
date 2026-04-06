# MyAthan Firmware — Claude Code Project Context

## Project Overview
Smart Islamic prayer time device firmware for ESP32-C3 SuperMini + DFPlayer Mini.
Fully offline prayer calculation, Hijri calendar, 7 Islamic holidays, Ramadan mode,
iqama timer, multi-room sync. BLE + WiFiManager provisioning.

## Tech Stack
- **Platform**: ESP32-C3 (RISC-V, 400KB RAM, 4MB flash, BLE 5.0, no Classic BT)
- **Build**: PlatformIO with Arduino framework
- **Storage**: LittleFS for config.json (v2 schema)
- **Audio**: DFPlayer Mini via Hardware UART1 (GPIO6/7, 9600 baud)
- **Libraries**: ArduinoJson 7, DFRobotDFPlayerMini, ESPAsyncWebServer, WiFiManager

## Architecture
- `src/config/` — ConfigManager with LittleFS JSON, v1→v2 migration
- `src/wifi/` — BLE GATT provisioning + WiFiManager captive portal fallback
- `src/time/` — NTP sync with configTzTime()
- `src/audio/` — DFPlayer driver with per-prayer volume and preview
- `src/led/` — 9-state non-blocking LED FSM
- `src/input/` — Debounced button (short/double/long press)
- `src/prayer/` — PrayerCalculator (7 methods), HijriCalendar, HolidayHandler, PrayerScheduler, IqamaTimer
- `src/api/` — ESPAsyncWebServer REST API at myathan.local
- `src/sync/` — Multi-room cloud-only NTP-aligned sync

## Key Decisions
- On-device prayer calculation is PRIMARY (works offline). Cloud is secondary.
- Power-loss recovery: skip and log (no replay after reboot)
- ASR: configurable Standard/Hanafi per device
- Suhoor alert: configurable (none/sound/led/custom)
- Multi-room: cloud-only sync via NTP-aligned epochs
- BT speaker: NOT supported (ESP32-C3 has BLE only, no A2DP)

## Config Schema
Config v2 at `data/config.json`. Key sections: location (method, asrJuristic, highLatitudeRule),
audio.prayers (per-prayer volume/iqamaDelay/ramadanTrack), schedule (fridayJumuah),
ramadan (suhoorMode), hijri (adjustment), holidays (7 holidays), multiRoom, recovery.

## Testing
- `test/test_prayer_calculator/` — 10 tests: cities, methods, high-latitude, ASR
- `test/test_hijri_calendar/` — 18 tests: known dates, holidays, adjustment, leap years

## Development
- Branch: `claude/firmware-implementation-plan-t2Mc1`
- Push: `git push -u origin claude/firmware-implementation-plan-t2Mc1`
- Build: `pio run -e esp32c3`
- Test: `pio test -e esp32c3`

## Related
- Core repo: `my-athan/core` (backend API, PWA, admin)
- GitHub issues: #10-#24 on firmware repo
