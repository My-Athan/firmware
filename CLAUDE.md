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

## Claude Model Guidance

| Task | Model | Why |
|------|-------|-----|
| Prayer calculation algorithms, high-latitude edge cases | **Opus** | Complex astronomical math with edge cases |
| Architecture changes, cross-repo schema updates | **Opus** | Cascading effects across firmware and core |
| Feature implementation, driver code, new modules | **Sonnet** | Standard embedded coding tasks |
| Bug fixes, config changes, single-file edits | **Sonnet** | Well-scoped changes |
| Code review with ESP32 safety checklist | **Sonnet** | Checklist-driven analysis |
| Build/flash/test commands, simple config tweaks | **Haiku** | Fast CLI execution |

## Common Development Patterns

1. **Prayer time change flow**: Edit `src/prayer/PrayerCalculator.cpp` → run `/test test_prayer_calculator` → run `/prayer-verify` for affected cities
2. **Config schema change flow**: Edit `data/config.json` + `src/config/ConfigManager.cpp` → add migration in `_migrateIfNeeded()` → update defaults in `_applyDefaults()` → notify core repo for shared type update
3. **New module flow**: Create `src/<module>/<Module>.h` + `.cpp` → add to `src/main.cpp` → write tests → run `/build` → run `/test`
4. **Audio change flow**: Edit `src/audio/AudioManager.cpp` → verify volume 0-30 constraint → test non-blocking playback

## Testing Strategy
- Before any commit: `pio test -e esp32c3`
- After prayer calculation changes: run `/prayer-verify` against Aladhan API for multiple cities
- After config changes: verify `CONFIG_MAX_SIZE` (8192) is sufficient
- After any change: run `/build` and check binary size (<1.5MB for OTA)

## Cross-Repo Sync
- `data/config.json` v2 schema must match core `DeviceConfig` in `packages/shared/`
- Prayer time output format must match core `PrayerTimes` type
- `IslamicHoliday` enum must match core enum
- Any config schema change requires updates in BOTH repos

## ESP32-C3 Quick Constraints
- 300KB usable RAM (400KB total) — no large buffers
- BLE 5.0 only — no Classic BT, no A2DP audio streaming
- GPIO6/7 = DFPlayer UART1, GPIO8 = LED, GPIO9 = Button
- OTA binary <=1.5MB, LittleFS config <=8192 bytes
- No SoftwareSerial — hardware UART only

## Related
- Core repo: `my-athan/core` (backend API, PWA, admin)
- GitHub issues: #10-#24 on firmware repo
