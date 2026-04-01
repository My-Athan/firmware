# MyAthan Firmware

Islamic prayer time device firmware for ESP32-C3 SuperMini with audio playback via DFPlayer Mini.

## Features

- **WiFi provisioning** via BLE or captive portal
- **Prayer time calculation** with Athan audio playback
- **LittleFS config storage** for device settings
- **OTA updates** with rollback safety via dual partitions
- **Local HTTP server** with mDNS discovery (`myathan.local`)
- **LED & button** for device status and manual control
- **Cloud sync** with backend for timetables, audio config, analytics

## Hardware

| Component | Specs | Cost |
|---|---|---|
| **ESP32-C3 SuperMini** | WiFi, BLE, 400KB RAM, 4MB flash | ~$1.50 |
| **DFPlayer Mini** | MP3/WAV playback, UART control | ~$1.00 |
| **Micro SD Card** | FAT32 formatted, up to 32GB | ~$2.00 |
| **5V Power Supply** | USB or barrel jack | varies |

## Pin Mapping (ESP32-C3 SuperMini)

| Function | GPIO | Notes |
|---|---|---|
| **UART0** (Serial/Logging) | TX=GPIO21, RX=GPIO20 | USB CDC debug output |
| **UART1** (DFPlayer) | TX=GPIO6, RX=GPIO7 | Hardware serial, 9600 baud |
| **LED** | GPIO8 | Status indicator (5x patterns) |
| **Button** | GPIO9 | Manual prayer trigger / reset |

## Build & Upload

### Prerequisites
- [PlatformIO Core](https://platformio.org/install/cli)
- Python 3.6+

### Compile
```bash
pio run -e esp32c3
```

### Upload to Device
```bash
pio run -t upload -e esp32c3
```

### Monitor Serial Output
```bash
pio device monitor -b 115200
```

## Configuration

Device config stored in **LittleFS** at `/config.json`. Edit via:

1. **Mobile PWA** (recommended) — WiFi provisioning, location, audio selection
2. **Local HTTP server** at `http://myathan.local` (LAN fallback)
3. **Direct LittleFS edit** (advanced) — via PlatformIO LittleFS filesystem

### Config Schema

```json
{
  "deviceId": "myathan-aabbcc",
  "firmwareVersion": "0.1.0",
  "configVersion": 1,
  "wifi": {
    "ssid": "",
    "password": ""
  },
  "location": {
    "lat": 0.0,
    "lon": 0.0,
    "city": "",
    "timezone": "UTC",
    "method": "ISNA"
  },
  "audio": {
    "volume": 20,
    "prayers": {
      "fajr": { "track": 1, "doaa": { "enabled": false, "track": 0, "delayMin": 3 } },
      "dhuhr": { "track": 1, "doaa": { "enabled": false, "track": 0, "delayMin": 3 } },
      "asr": { "track": 1, "doaa": { "enabled": false, "track": 0, "delayMin": 3 } },
      "maghrib": { "track": 1, "doaa": { "enabled": false, "track": 0, "delayMin": 3 } },
      "isha": { "track": 1, "doaa": { "enabled": false, "track": 0, "delayMin": 3 } }
    }
  },
  "led": {
    "enabled": true,
    "preAthanMinutes": 10,
    "preAthanPattern": "slow_blink",
    "playingPattern": "solid",
    "errorPattern": "fast_blink",
    "noWifiPattern": "pulse"
  },
  "ota": {
    "checkHour": 3,
    "lastChecked": ""
  }
}
```

## Architecture

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for:
- System architecture diagram
- Hardware selection rationale
- Communication protocols (BLE, HTTP, HTTPS, mDNS)
- OTA update mechanism
- Security hardening
- Disaster recovery plan

## Development Phases

See [`docs/PROJECT.md`](docs/PROJECT.md) for full implementation backlog with epics and stories:

- **Phase 1**: Firmware foundation (WiFi provisioning, prayer scheduler, LED/button, local HTTP)
- **Phase 2**: Backend + device connectivity (timetable API, config sync, analytics)
- **Phase 3**: OTA updates (CI/CD, rollback, staged rollouts)
- **Phase 4**: Mobile PWA (setup, control, statistics)
- **Phase 5**: Admin panel (fleet management, audio catalog, remote config)
- **Phase 6**: Polish & v2 (email alerts, multi-device, scalability)

## Contributing

Firmware code lives on `claude/myathan-device-setup-*` feature branches. 

- All changes commit to feature branch, push with `-u origin <branch>`
- Documentation (architecture, project plan, API specs) lives in `docs/` on main
- GitHub issues (#2-#8) track implementation progress

## License

Copyright 2026 MyAthan Contributors. See LICENSE file.

## Support

- 📖 [Architecture Plan](docs/ARCHITECTURE.md)
- 📋 [Project Backlog](docs/PROJECT.md)
- 🛠️ [CI/CD Setup](docs/GITHUB_SETUP.md)
- 💬 GitHub Issues: https://github.com/My-Athan/firmware/issues
