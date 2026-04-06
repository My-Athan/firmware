# MyAthan Firmware

Smart Islamic prayer time device firmware for ESP32-C3 SuperMini with DFPlayer Mini audio.

**Status: Phase 1 Complete** — All 16 firmware components implemented.

## Features

### Core (Phase 1A)
- **WiFi provisioning** — BLE GATT service + WiFiManager captive portal fallback
- **NTP sync** — timezone-aware time synchronization
- **Audio playback** — DFPlayer Mini driver with per-prayer volume and 10s preview mode
- **LED indicators** — 9-state non-blocking FSM (idle, pre-athan, playing, iqama, error, etc.)
- **Button control** — short press (trigger), double press (preview), long press (WiFi reset)

### Prayer Engine (Phase 1B)
- **On-device prayer calculation** — PrayTimes.org port, works fully offline
  - 7 methods: ISNA, MWL, Egypt, Makkah, Karachi, Tehran, Jafari
  - ASR: configurable Standard (Shafi'i) or Hanafi
  - 3 high-latitude methods: angle-based, midnight, one-seventh
- **Hijri calendar** — tabular algorithm, +/-2 day manual adjustment
- **7 Islamic holidays** — Eid al-Fitr, Eid al-Adha, Mawlid, Isra & Mi'raj, Muharram, Ashura, Laylat al-Qadr
  - Per-holiday configurable actions (special track, post-athan takbeer/doaa)
- **Friday/Jumuah** — replaces Dhuhr athan with Jumuah track on Fridays
- **Ramadan mode** — auto-detected via Hijri calendar
  - Special Fajr athan, suhoor alert (configurable: none/sound/LED/custom), iftar alert
- **Scheduled volume** — per-prayer volume levels (quieter Fajr)
- **Iqama timer** — per-prayer countdown (0-60 min) with accelerating LED pulse
- **Power-loss recovery** — skip and log (no replay after reboot)

### Server & Sync (Phase 1C)
- **REST API** at `myathan.local` — 8 endpoints (status, timetable, trigger, preview, volume, config, sync)
- **Multi-room sync** — cloud-only NTP-aligned group playback

## Hardware

| Component | Specs | Cost |
|---|---|---|
| ESP32-C3 SuperMini | WiFi + BLE 5.0, 400KB RAM, 4MB flash | ~$1.50 |
| DFPlayer Mini | MP3/WAV, UART, 3W amp, microSD | ~$1.00 |
| Micro SD Card | FAT32, up to 32GB | ~$1.00 |
| Speaker | 4Ω 3W | ~$0.50 |
| LED + Button | GPIO8 / GPIO9 | ~$0.15 |
| **Total** | | **~$4.15** |

## Pin Mapping

| Function | GPIO | Notes |
|---|---|---|
| UART0 (Serial/Logging) | TX=21, RX=20 | USB CDC debug |
| UART1 (DFPlayer) | TX=6, RX=7 | Hardware serial, 9600 baud |
| LED | 8 | Status indicator (9 patterns) |
| Button | 9 | INPUT_PULLUP, active LOW |

## Project Structure

```
firmware/
├── src/
│   ├── main.cpp                    # Entry point, wires all managers
│   ├── config/
│   │   ├── ConfigManager.h/.cpp    # LittleFS JSON config (v2 with migration)
│   │   └── defaults.h              # All default constants
│   ├── wifi/
│   │   └── WifiProvisioner.h/.cpp  # BLE + WiFiManager provisioning
│   ├── time/
│   │   └── NtpSync.h/.cpp          # Timezone-aware NTP
│   ├── audio/
│   │   └── AudioManager.h/.cpp     # DFPlayer UART1 driver
│   ├── led/
│   │   └── LedManager.h/.cpp       # 9-state non-blocking LED FSM
│   ├── input/
│   │   └── ButtonHandler.h/.cpp    # Debounced short/double/long press
│   ├── prayer/
│   │   ├── PrayerCalculator.h/.cpp # On-device prayer times (7 methods)
│   │   ├── HijriCalendar.h/.cpp    # Tabular Hijri conversion
│   │   ├── HolidayHandler.h/.cpp   # 7 Islamic holidays
│   │   ├── PrayerScheduler.h/.cpp  # Master scheduler
│   │   └── IqamaTimer.h/.cpp       # Per-prayer countdown
│   ├── api/
│   │   └── LocalServer.h/.cpp      # ESPAsyncWebServer REST API
│   └── sync/
│       └── MultiRoomSync.h/.cpp    # Cloud-only group sync
├── data/
│   └── config.json                 # Default config (v2 schema)
├── test/
│   ├── test_prayer_calculator/     # Prayer time accuracy tests
│   └── test_hijri_calendar/        # Hijri conversion + holiday tests
├── include/
│   └── version.h                   # Firmware version (1.0.0)
└── platformio.ini                  # Build configuration
```

## REST API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/status` | Device state, prayer times, Hijri date, Ramadan status |
| GET | `/timetable` | Today + tomorrow calculated prayer times |
| POST | `/trigger?prayer=N` | Manual athan trigger (0=Fajr ... 4=Isha) |
| POST | `/preview?track=N` | Play 10-second preview of track |
| POST | `/volume?level=N` | Set volume (0-30) |
| GET | `/config` | Read full device config |
| POST | `/config` | Partial config merge (JSON body) |
| POST | `/sync-trigger` | Multi-room sync trigger |

## Build & Upload

```bash
# Compile
pio run -e esp32c3

# Upload to device
pio run -t upload -e esp32c3

# Upload LittleFS filesystem
pio run -t uploadfs -e esp32c3

# Monitor serial
pio device monitor -b 115200

# Run unit tests
pio test -e esp32c3
```

## Config Schema (v2)

Full schema at [`data/config.json`](data/config.json). Key sections:

- `location` — lat/lon, timezone, calculation method, ASR juristic, high-latitude rule
- `audio.prayers.<prayer>` — track, volume, iqamaDelay, ramadanTrack, doaa
- `schedule` — fridayJumuah, jumuahTrack
- `ramadan` — suhoorAlertMinutes, suhoorMode (none/sound/led/custom), iftarTrack
- `hijri` — adjustment (-2 to +2 days)
- `holidays` — 7 holidays with enabled/track/postAthanTrack
- `multiRoom` — groupId, syncOffsetMs
- `recovery` — lastState, lastPrayerPlayed, playedTodayMask

## Development Phases

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Firmware Foundation (16 components) | ✅ Complete |
| 2 | Backend + Device Connectivity | 🔲 TODO |
| 3 | OTA Update System | 🔲 TODO |
| 4 | Mobile PWA | 🔲 TODO |
| 5 | Admin Dashboard | 🔲 TODO |
| 6 | Polish & v2 | 🔲 TODO |

## Related

- [Core repo](https://github.com/My-Athan/core) — Backend API, PWA, admin dashboard
- [Architecture](docs/ARCHITECTURE.md) — System design, security, disaster recovery
- [Project backlog](docs/PROJECT.md) — All epics and stories
- [Issues](https://github.com/My-Athan/firmware/issues) — Implementation tracking
