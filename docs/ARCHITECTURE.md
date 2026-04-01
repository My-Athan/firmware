# MyAthan Device Ecosystem — Comprehensive Plan

## Context
MyAthan is an Islamic prayer time (Athan/Adhan) device. The ecosystem has 3 components: firmware on the device, a mobile PWA for configuration, and a backend API for timetables, OTA updates, and analytics.

The original hardware (ESP8266 + JQ6500 2MB) was evaluated and found insufficient: JQ6500's 2MB flash can barely hold one athan file, cannot be written programmatically, and ESP8266 lacks BLE and has limited RAM for HTTPS. We are upgrading to **ESP32-C3 + DFPlayer Mini + micro SD** at roughly the same cost (~$4-5 BOM).

The ecosystem has **4 components**: firmware, mobile PWA, backend API, and **admin panel** for fleet management.

---

## Hardware BOM

| Component | Model | Cost | Purpose |
|---|---|---|---|
| MCU | ESP32-C3 SuperMini | ~$1.50 | WiFi + BLE, 400KB RAM, 4MB flash, hardware TLS |
| Audio | DFPlayer Mini | ~$1.00 | MP3/WAV playback via UART, micro SD slot |
| Storage | Micro SD 2GB | ~$1.00 | Athan/doaa audio files (hundreds of files) |
| LED | Single RGB (WS2812B) or plain LED | ~$0.10 | Status indication |
| Speaker | 8Ω 2W | ~$0.50 | Audio output |
| Button | Tactile switch | ~$0.05 | WiFi reset / manual trigger |
| **Total** | | **~$4.15** | |

### Why ESP32-C3 over ESP8266
- Same price (~$1.50)
- 400KB RAM vs 80KB → room for HTTPS, JSON parsing, web server
- BLE support → enables BLE WiFi provisioning from app (smoother UX than captive portal)
- Hardware-accelerated TLS → fast, reliable HTTPS to backend
- Native dual OTA partitions → reliable firmware rollback
- More GPIOs (15 vs 11)

### Why DFPlayer Mini over JQ6500
- Micro SD card support → unlimited audio files vs 2MB fixed flash
- ESP32 can write files to SD via SPI → future OTA audio file download
- Same UART serial protocol → nearly identical firmware code
- Actually cheaper (~$1.00 vs ~$1.50)

---

## Architecture Overview

```
┌─────────────┐     HTTPS      ┌──────────────┐     HTTPS      ┌─────────────────┐
│  Mobile PWA │ ◄────────────► │  Backend API  │ ◄────────────► │  ESP32-C3 Device│
│  (React)    │                │  (Fastify)    │                │  + DFPlayer Mini│
└─────────────┘                └──────┬────────┘                └────────┬────────┘
       │                              │                                  │
       │    BLE / Local HTTP          │                                  │
       └──────────────────────────────┼──────────────────────────────────┘
                                      │
                              ┌───────┴────────┐
                              │  PostgreSQL    │
                              │  Cloudflare R2 │
                              │  (files/bins)  │
                              └───────┬────────┘
                                      │
                              ┌───────┴────────┐
                              │  Admin Panel   │
                              │  (React SPA)   │
                              └────────────────┘
```

### Communication Strategy: Hybrid (3 channels)

| Channel | When | Protocol | Purpose |
|---|---|---|---|
| **BLE** | First setup | BLE GATT | WiFi provisioning from app (no captive portal needed) |
| **Local HTTP** | Same LAN | HTTP to `myathan.local` (mDNS) | Volume, manual trigger, status, WiFi change |
| **Cloud** | Always | HTTPS polling (device → BE every 5 min) | Timetable, config sync, OTA, stats, heartbeat |

**Flow:**
1. User opens PWA → BLE scan finds "MyAthan-XXXX" → sends WiFi credentials over BLE → device connects to WiFi
2. Device registers with backend (UUID + first heartbeat)
3. Device fetches timetable + config from backend, stores locally
4. App controls device via backend (config sync) or direct HTTP on same LAN
5. Device checks for OTA daily at 3AM

---

## Technology Stack

### 1. Firmware (ESP32-C3) — `My-Athan/firmware` (this repo)

| Component | Choice | Reason |
|---|---|---|
| Build system | **PlatformIO** | Dependency management, CI-friendly, multi-env |
| Framework | **Arduino for ESP32** | Mature, huge library ecosystem |
| WiFi provisioning | **BLE + WiFiProv** (ESP-IDF native) | Smooth app-driven setup, no captive portal |
| Fallback provisioning | **WiFiManager** (captive portal) | Backup if BLE fails or user has no app |
| OTA updates | **HTTP OTA** (custom check against BE) | Device pulls binary from R2 via HTTPS |
| Time sync | **NTP** (configTzTime built-in) | Timezone-aware, reliable |
| Config storage | **LittleFS** | JSON config files, wear-leveled |
| HTTP client | **HTTPClient** (ESP32 built-in) | HTTPS with hardware TLS |
| Audio | **DFRobotDFPlayerMini** library | UART control of DFPlayer Mini |
| JSON | **ArduinoJson v7** | Latest, optimized for ESP32 |
| LED | **Non-blocking millis() state machine** | No blocking delays |
| Local web server | **ESPAsyncWebServer** | Concurrent requests, mDNS |
| Scheduler | **Custom minute-check loop** | Compare current time against prayer times each minute |

### 2. Mobile App (PWA) — `My-Athan/app` (new repo)

| Component | Choice | Reason |
|---|---|---|
| Framework | **React + Vite** | Fast builds, PWA-ready, lightweight |
| UI | **Tailwind CSS + shadcn/ui** | Mobile-first, minimal bundle |
| PWA | **Vite PWA plugin** | Offline shell, installable |
| BLE | **Web Bluetooth API** | Native browser BLE for WiFi provisioning |
| State | **Zustand** | Tiny, no boilerplate |
| API | **TanStack Query** | Caching, background refetch |
| Local device | **Fetch API to `myathan.local`** | Direct LAN control |

**Why PWA is the right call (even more so with ESP32-C3):**
- Web Bluetooth API enables BLE WiFi provisioning directly from browser — no native app needed
- No app store friction, instant updates, cross-platform
- `myathan.local` mDNS works for local HTTP control on same network
- Only limitation: Web Bluetooth requires Chrome/Edge (not Safari/Firefox). Fallback: captive portal WiFi setup works on any browser.

**Mobile app lives in**: `My-Athan/web` → `packages/app/`

### 3. Backend API — `My-Athan/backend` (new repo)

| Component | Choice | Reason |
|---|---|---|
| Runtime | **Node.js + Fastify** | Fast, TypeScript-native |
| Language | **TypeScript** | Shared types with PWA + admin |
| Database | **PostgreSQL** | Relational: devices, users, configs, stats |
| ORM | **Drizzle ORM** | Lightweight, type-safe |
| Prayer times | **adhan-js** | Local calculation (ISNA/MWL/Umm Al-Qura methods) |
| IP geolocation | **ip-api.com** (free) or **MaxMind GeoLite2** | IP → lat/lon for auto-location |
| File storage | **Cloudflare R2** | S3-compatible, free egress, CDN |
| Auth | **API key** (device), **JWT** (app + admin users) | Simple + secure |
| Hosting | **Hostinger VPS + Coolify** | Self-hosted, cost-effective, full control |

### 4. Admin Panel — `My-Athan/admin` (new repo)

| Component | Choice | Reason |
|---|---|---|
| Framework | **React + Vite** | Same stack as mobile PWA, shared knowledge |
| UI | **MUI (Material UI)** | Licensed; comprehensive component library for data-heavy dashboards |
| Data tables | **MUI DataGrid Pro** | Built-in sort, filter, pagination, column resize, row selection, export |
| Charts | **MUI X Charts** | Included in Pro license; consistent with MUI design system |
| Forms | **MUI form components** | TextField, Select, Autocomplete, DatePicker — polished and consistent |
| State | **Zustand** | Consistent with mobile app |
| API | **TanStack Query** | Same as mobile app |
| Auth | **JWT (admin role)** | Role-based: admin vs regular user |

**Admin panel is a separate SPA** (not a PWA — desktop-first, no offline needed). Uses **MUI** for the rich data table/form/chart experience. The mobile PWA stays on Tailwind + shadcn for lightweight bundle size.

**Admin panel lives in**: `My-Athan/web` → `packages/admin/`

---

## Feature Evaluation & Final List

### Firmware Features

| # | Feature | Decision | Implementation |
|---|---|---|---|
| F1 | WiFi provisioning | ✅ Keep | BLE GATT primary + WiFiManager captive portal fallback |
| F2 | OTA updates | ✅ Keep | Daily 3AM check → HTTPS download → SHA256 verify → flash |
| F3 | Per-prayer audio | ✅ Keep | SD card tracks mapped per prayer in config (track 1-5 = 5 prayers) |
| F4 | Default athan | ✅ Keep | Config default track; individual prayers override |
| F5 | Doaa after athan | ✅ Keep | Configurable per prayer: enable/disable, track, delay minutes |
| F6 | LED status | ✅ Keep | State machine: idle=off, pre-athan=slow_blink, playing=solid, error=fast_blink, no-wifi=pulse |
| F7 | Volume control | ✅ Keep | Stored in config, DFPlayer serial command, controllable via local API + app |
| F8 | WiFi reset | ✅ Keep | Hold button 5s → clear credentials → reboot into BLE/AP mode |
| F9 | Manual athan trigger | ✅ Keep | Button short press OR local HTTP `POST /trigger` |
| F10 | Statistics | ✅ Keep (simplified) | Daily batch: play counts per prayer, errors, uptime, free heap |
| F11 | **Offline fallback** | ✅ Add | Cache last 7 days timetable in LittleFS; calculate locally if no internet for >7 days |
| F12 | **Heartbeat** | ✅ Add | Hourly ping to BE; enables device-online monitoring |
| F13 | **Audio file OTA** | ✅ Add (v2) | ESP32 downloads MP3 from R2 → writes to SD card via SPI |
| F14 | ~~Real-time streaming~~ | ❌ Remove | DFPlayer plays from SD, not streaming. v2 could add I2S DAC |

### Mobile App Features

| # | Feature | Decision | Implementation |
|---|---|---|---|
| A1 | Device setup (WiFi) | ✅ Keep | Web Bluetooth BLE pairing → send WiFi creds |
| A2 | Location change | ✅ Keep | City search / map pin → POST to BE → device fetches new timetable |
| A3 | Manual timetable override | ✅ Keep | Per-prayer ±30 min offset slider |
| A4 | Change WiFi | ✅ Keep | Via local HTTP API (must be on same network) |
| A5 | LED behavior config | ✅ Keep | Toggle patterns, pre-athan lead time (5/10/15 min) |
| A6 | Per-prayer athan file | ✅ Keep | Select from audio catalog per prayer |
| A7 | Doaa configuration | ✅ Keep | Enable per prayer + select doaa file + delay |
| A8 | Statistics view | ✅ Keep | Simple dashboard: plays, last seen, uptime |
| A9 | **Device pairing** | ✅ Add | BLE scan finds device → pair with user account |
| A10 | **Audio file upload** | ✅ Add | Upload MP3 to BE → available in catalog |
| A11 | **Calculation method** | ✅ Add | Select prayer time method (ISNA, MWL, Umm Al-Qura, Egyptian, etc.) |

### Backend Features

| # | Feature | Decision | Implementation |
|---|---|---|---|
| B1 | Timetable by IP | ✅ Keep | `GET /api/v1/timetable` → IP geolocation → adhan-js calculation |
| B2 | Timetable by coords | ✅ Keep | `GET /api/v1/timetable?lat=&lon=&method=` |
| B3 | Config sync | ✅ Add | Device polls `GET /config/:id`; app writes `POST /config/:id` |
| B4 | OTA releases | ✅ Keep | Admin uploads binary → `GET /releases/latest` for device |
| B5 | Device registry | ✅ Add | UUID → user mapping, heartbeat tracking |
| B6 | Statistics collection | ✅ Keep | `POST /stats` from device; analytics dashboard |
| B7 | Audio file catalog | ✅ Add | Upload/manage athan and doaa files in R2 |
| B8 | ~~Streaming~~ | ❌ Remove | Not needed with SD card |
| B9 | **Admin auth + RBAC** | ✅ Add | Admin role in JWT; middleware guards admin endpoints |
| B10 | **Bulk device operations** | ✅ Add | Push config/OTA to multiple devices at once |
| B11 | **Remote audio change** | ✅ Add | Admin sets audio config per device → device picks up on next poll |

### Admin Panel Features

| # | Feature | Decision | Implementation |
|---|---|---|---|
| D1 | **Device fleet table** | ✅ Add | Sortable/filterable table: device ID, status (online/offline), firmware version, location, last heartbeat, owner |
| D2 | **Online/offline status** | ✅ Add | Green/red indicator based on last heartbeat (online = heartbeat within 2 hours) |
| D3 | **Device detail view** | ✅ Add | Click row → full device info: config, timetable, play history, error log, uptime chart |
| D4 | **Firmware management** | ✅ Add | Upload new firmware binary, see rollout status (how many devices updated, pending, failed) |
| D5 | **Push OTA to device(s)** | ✅ Add | Select device(s) → force OTA check on next poll; or schedule rollout |
| D6 | **Remote audio config** | ✅ Add | Select device → change athan track per prayer + doaa config → pushed via config sync |
| D7 | **Audio file management** | ✅ Add | Upload/delete athan and doaa MP3 files to R2; manage catalog visible to all users |
| D8 | **Aggregate analytics** | ✅ Add | Dashboard: total devices, active vs inactive, plays per day chart, geographic distribution, error rates |
| D9 | **Device statistics** | ✅ Add | Per-device: daily play count per prayer, missed athans, connectivity gaps, errors |
| D10 | **Bulk config push** | ✅ Add | Select multiple devices → push same config (e.g., change default athan for all devices) |
| D11 | **Alert management** | ✅ Add | View/configure alerts: device offline >24h, OTA failure, repeated errors |

---

## Repository Structure

### Three repos (recommended)
```
My-Athan/firmware     ← this repo (PlatformIO, C++)
My-Athan/backend      ← new repo (Node.js/Fastify, TypeScript)
My-Athan/web          ← new repo (pnpm monorepo: Mobile PWA + Admin Panel)
```

**Why 3 not 4**: Mobile PWA and Admin Panel share React + Vite + TypeScript, API client code, auth logic, types, and TanStack Query hooks. A pnpm workspace monorepo with Turborepo keeps them in sync while building independently.

### Firmware Repo Structure (this repo)
```
firmware/
├── platformio.ini                  # ESP32-C3 SuperMini target
├── include/
│   └── version.h                   # BUILD_VERSION (auto-set in CI)
├── src/
│   ├── main.cpp                    # Setup + main loop
│   ├── config/
│   │   ├── ConfigManager.h/.cpp    # LittleFS JSON read/write
│   │   └── defaults.h              # Default config values
│   ├── wifi/
│   │   ├── WifiProvisioner.h/.cpp  # BLE provisioning + WiFiManager fallback
│   │   └── WifiReset.h/.cpp        # Button-triggered reset
│   ├── audio/
│   │   ├── AudioManager.h/.cpp     # DFPlayer wrapper (play, stop, volume, track select)
│   │   └── AudioScheduler.h/.cpp   # Maps prayer times → track playback + doaa delay
│   ├── led/
│   │   └── LedManager.h/.cpp       # Non-blocking state machine (patterns)
│   ├── time/
│   │   ├── NtpSync.h/.cpp          # NTP + timezone config
│   │   └── PrayerScheduler.h/.cpp  # Minute-by-minute prayer time check
│   ├── ota/
│   │   └── OtaManager.h/.cpp       # HTTP check + download + verify + flash
│   ├── api/
│   │   └── LocalServer.h/.cpp      # ESPAsyncWebServer: /status, /trigger, /volume, /config
│   ├── net/
│   │   └── BackendClient.h/.cpp    # HTTPS: timetable, config, heartbeat, stats
│   └── stats/
│       └── StatsCollector.h/.cpp   # Accumulate + batch upload daily
├── lib/                            # Vendored/custom libraries
├── data/                           # LittleFS filesystem image (default config)
├── test/                           # PlatformIO unit tests
└── .github/
    └── workflows/
        └── build.yml               # CI: build + upload binary to BE on tag
```

### Web Repo Structure (`My-Athan/web` — pnpm monorepo)
```
web/
├── package.json                    # Workspace root
├── pnpm-workspace.yaml             # Workspace config
├── turbo.json                      # Turborepo build orchestration
├── packages/
│   ├── shared/                     # Shared package
│   │   ├── package.json
│   │   ├── src/
│   │   │   ├── api/                # API client (Axios/fetch wrapper)
│   │   │   │   ├── client.ts       # Base HTTP client with auth headers
│   │   │   │   ├── devices.ts      # Device API hooks
│   │   │   │   ├── timetable.ts    # Timetable API hooks
│   │   │   │   └── audio.ts        # Audio file API hooks
│   │   │   ├── types/              # Shared TypeScript interfaces
│   │   │   │   ├── device.ts       # Device, Config, Heartbeat types
│   │   │   │   ├── timetable.ts    # Prayer times, Location types
│   │   │   │   ├── audio.ts        # Audio file, track mapping types
│   │   │   │   └── stats.ts        # Statistics, analytics types
│   │   │   ├── auth/               # JWT auth logic (login, token refresh)
│   │   │   └── hooks/              # Shared TanStack Query hooks
│   │   └── tsconfig.json
│   ├── app/                        # Mobile PWA
│   │   ├── package.json
│   │   ├── vite.config.ts          # Vite + PWA plugin
│   │   ├── src/
│   │   │   ├── main.tsx
│   │   │   ├── pages/              # Timetable, Setup, Settings, Stats
│   │   │   ├── components/         # shadcn/ui components
│   │   │   ├── ble/                # Web Bluetooth provisioning
│   │   │   └── stores/             # Zustand stores
│   │   ├── public/
│   │   │   └── manifest.json       # PWA manifest
│   │   └── tailwind.config.ts
│   └── admin/                      # Admin Panel
│       ├── package.json
│       ├── vite.config.ts
│       ├── src/
│       │   ├── main.tsx
│       │   ├── pages/
│       │   │   ├── Dashboard.tsx    # Aggregate analytics charts
│       │   │   ├── Devices.tsx      # MUI DataGrid fleet table
│       │   │   ├── DeviceDetail.tsx # Config, stats, heartbeat history
│       │   │   ├── Releases.tsx     # Firmware upload + rollout status
│       │   │   ├── AudioFiles.tsx   # Audio catalog management
│       │   │   └── Alerts.tsx       # Alert management
│       │   ├── components/         # MUI-based components
│       │   └── stores/             # Zustand stores
│       └── tsconfig.json
└── .github/
    └── workflows/
        ├── app.yml                 # CI: build + deploy PWA
        └── admin.yml               # CI: build + deploy admin
```

### Backend Repo Structure (`My-Athan/backend`)
```
backend/
├── package.json
├── tsconfig.json
├── drizzle.config.ts               # Drizzle ORM config
├── src/
│   ├── index.ts                    # Fastify server entry
│   ├── plugins/                    # Fastify plugins (auth, cors, etc.)
│   ├── routes/
│   │   ├── device/                 # Device API: timetable, config, heartbeat, stats
│   │   ├── app/                    # App API: auth, devices, audio-files
│   │   └── admin/                  # Admin API: fleet, releases, analytics, alerts
│   ├── services/
│   │   ├── prayer-times.ts         # adhan-js wrapper
│   │   ├── geolocation.ts          # IP → lat/lon
│   │   ├── ota.ts                  # Release management + R2 upload
│   │   └── analytics.ts            # Stats aggregation
│   ├── db/
│   │   ├── schema.ts               # Drizzle schema (devices, users, configs, stats, releases)
│   │   └── migrations/             # SQL migrations
│   └── lib/
│       ├── auth.ts                 # JWT + API key verification
│       └── r2.ts                   # Cloudflare R2 client
├── test/                           # Vitest tests
└── .github/
    └── workflows/
        └── deploy.yml              # CI: test + deploy to Railway/Fly.io
```

---

## Device Config Schema (LittleFS `/config.json`)

```json
{
  "deviceId": "uuid-v4",
  "firmwareVersion": "1.0.0",
  "wifi": { "ssid": "", "password": "" },
  "location": {
    "lat": 0.0, "lon": 0.0,
    "city": "", "country": "",
    "timezone": "UTC",
    "method": "ISNA"
  },
  "audio": {
    "volume": 20,
    "defaultTrack": 1,
    "prayers": {
      "fajr":    { "track": 1, "enabled": true,  "doaa": { "enabled": true,  "track": 6, "delayMin": 3 } },
      "dhuhr":   { "track": 2, "enabled": true,  "doaa": { "enabled": false } },
      "asr":     { "track": 2, "enabled": true,  "doaa": { "enabled": false } },
      "maghrib": { "track": 3, "enabled": true,  "doaa": { "enabled": true,  "track": 7, "delayMin": 2 } },
      "isha":    { "track": 2, "enabled": true,  "doaa": { "enabled": false } }
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
  "timetable": {
    "fetchedAt": "2026-03-31",
    "offsets": { "fajr": 0, "dhuhr": 0, "asr": 0, "maghrib": 0, "isha": 0 },
    "days": {
      "2026-03-31": { "fajr": "05:12", "sunrise": "06:30", "dhuhr": "12:30", "asr": "15:45", "maghrib": "18:22", "isha": "19:50" }
    }
  },
  "ota": { "checkHour": 3, "lastChecked": "" },
  "stats": { "lastSent": "" }
}
```

---

## Backend API Endpoints

### Device API (`X-Device-Key` header auth)
```
GET  /api/v1/timetable                        # auto-IP geolocation + method from config
GET  /api/v1/timetable?lat=&lon=&method=&days=7  # explicit, returns 7 days
GET  /api/v1/config/:deviceId                  # pull latest config (device polls)
POST /api/v1/heartbeat                         # { deviceId, version, freeHeap, uptime }
POST /api/v1/stats                             # { deviceId, plays: {...}, errors: [...] }
GET  /api/v1/releases/latest?platform=esp32c3  # { version, url, sha256, size }
GET  /api/v1/audio/:fileId/download            # download audio file (for SD card OTA v2)
```

### App API (JWT auth)
```
POST /api/v1/auth/register
POST /api/v1/auth/login
POST /api/v1/devices/pair                      # { deviceId } → link to user
GET  /api/v1/devices                           # list user's devices
GET  /api/v1/devices/:id                       # device status + last heartbeat
POST /api/v1/devices/:id/config                # push config update (app → BE → device polls)
GET  /api/v1/analytics/:deviceId               # usage stats
GET  /api/v1/audio-files                       # list athan/doaa catalog
POST /api/v1/audio-files                       # upload new audio file
```

### Admin API (admin JWT — used by admin panel)
```
# Auth
POST /admin/auth/login                         # admin login → JWT with admin role

# Device Fleet Management
GET  /admin/devices                            # paginated device list (sortable, filterable)
GET  /admin/devices?status=online&version=1.0  # filter by status, version, location, etc.
GET  /admin/devices/:id                        # full device detail (config, stats, heartbeat history)
PUT  /admin/devices/:id/config                 # push config change to device (audio, LED, etc.)
POST /admin/devices/bulk/config                # push same config to multiple devices { deviceIds, config }
POST /admin/devices/:id/force-ota              # flag device for immediate OTA on next poll

# Firmware Releases
GET  /admin/releases                           # list all releases with rollout stats
POST /admin/releases                           # upload new firmware binary + metadata
GET  /admin/releases/:id/status                # rollout status: updated/pending/failed counts

# Audio File Management
GET  /admin/audio-files                        # list all audio files in catalog
POST /admin/audio-files                        # upload new athan/doaa MP3 to R2
DELETE /admin/audio-files/:id                  # remove audio file

# Analytics & Monitoring
GET  /admin/analytics                          # aggregate: total devices, active, plays/day, geo distribution
GET  /admin/analytics/devices/:id              # per-device stats: plays, missed, errors, uptime
GET  /admin/alerts                             # list active alerts (offline, OTA fail, errors)
PUT  /admin/alerts/:id/acknowledge             # acknowledge/dismiss alert
```

---

## OTA Update Flow

1. Timer fires at configured hour (default 3AM)
2. `GET /api/v1/releases/latest?platform=esp32c3` → `{ version: "1.2.0", url: "...", sha256: "..." }`
3. Compare with `BUILD_VERSION` compiled into firmware
4. If newer: HTTPS download binary from R2 URL → verify SHA256 → `Update.begin()` → `Update.write()` → `Update.end()` → reboot
5. Post-reboot: device sends heartbeat with new version; if it doesn't heartbeat within 120s, ESP32 boots previous partition (dual OTA rollback)

---

## Infrastructure — Hostinger VPS + Coolify

### Overview
All server-side components run on a **Hostinger VPS** managed by **Coolify** (open-source self-hosted PaaS, Heroku/Railway alternative). Coolify handles Docker deployments, SSL, reverse proxy, and database management.

### Hostinger VPS Requirements
| Spec | Minimum | Recommended |
|---|---|---|
| CPU | 2 vCPU | 4 vCPU |
| RAM | 4 GB | 8 GB |
| Storage | 80 GB NVMe | 160 GB NVMe |
| OS | Ubuntu 22.04 LTS | Ubuntu 24.04 LTS |
| Location | Closest to target users | EU or Middle East |

### What Runs on the VPS (via Coolify)

```
Hostinger VPS (Coolify-managed)
├── Coolify Dashboard         (port 8000, admin UI for managing deployments)
├── Traefik Reverse Proxy     (auto-managed by Coolify, handles SSL + routing)
│
├── Backend API               (Docker container, Fastify Node.js app)
│   └── api.myathan.com       (Traefik route, auto SSL via Let's Encrypt)
│
├── Mobile PWA                (Docker container, Nginx serving static Vite build)
│   └── app.myathan.com       (Traefik route)
│
├── Admin Panel               (Docker container, Nginx serving static Vite build)
│   └── admin.myathan.com     (Traefik route)
│
├── PostgreSQL 16             (Docker container, managed by Coolify)
│   └── Internal network only (not exposed to internet)
│
└── Backups                   (Coolify scheduled backups → S3/R2)
```

### Domain & DNS Setup
| Domain | Points To | Purpose |
|---|---|---|
| `myathan.com` | Hostinger VPS IP | Landing page (future) |
| `api.myathan.com` | Hostinger VPS IP | Backend API (device + app + admin) |
| `app.myathan.com` | Hostinger VPS IP | Mobile PWA |
| `admin.myathan.com` | Hostinger VPS IP | Admin panel |

All DNS A records point to the VPS IP. Coolify's Traefik handles routing by hostname and provisions SSL certificates via Let's Encrypt automatically.

### Coolify Setup Steps
1. **Install Coolify** on the VPS: `curl -fsSL https://cdn.coollabs.io/coolify/install.sh | bash`
2. **Configure DNS**: Point `api.myathan.com`, `app.myathan.com`, `admin.myathan.com` to VPS IP
3. **Create resources in Coolify**:
   - **PostgreSQL**: Add new database resource → PostgreSQL 16 → set credentials → Coolify manages the container
   - **Backend API**: Add new service → connect `My-Athan/backend` repo → set build pack (Dockerfile) → env vars (DB_URL, R2 keys, JWT secret) → deploy
   - **Mobile PWA**: Add new service → connect `My-Athan/web` → build command `cd packages/app && pnpm build` → serve from `dist/` via Nginx
   - **Admin Panel**: Add new service → connect `My-Athan/web` → build command `cd packages/admin && pnpm build` → serve from `dist/` via Nginx
4. **Enable auto-deploy**: Coolify webhook on GitHub push → auto-rebuild + zero-downtime deploy
5. **Configure backups**: Coolify → Settings → Backups → schedule daily PostgreSQL dump → upload to Cloudflare R2

### Coolify Deployment Config (per service)

**Backend API** (`Dockerfile`):
```dockerfile
FROM node:20-alpine
WORKDIR /app
COPY package.json pnpm-lock.yaml ./
RUN corepack enable && pnpm install --frozen-lockfile
COPY . .
RUN pnpm build
EXPOSE 3000
CMD ["node", "dist/index.js"]
```

**PWA / Admin** (static Nginx):
```dockerfile
FROM node:20-alpine AS build
WORKDIR /app
COPY . .
RUN corepack enable && pnpm install --frozen-lockfile
RUN pnpm --filter app build  # or --filter admin
FROM nginx:alpine
COPY --from=build /app/packages/app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf
```

### Environment Variables (Backend)
```
DATABASE_URL=postgresql://myathan:***@postgres:5432/myathan
JWT_SECRET=<generated-secret>
DEVICE_API_KEY_SALT=<generated-salt>
R2_ACCOUNT_ID=<cloudflare-account-id>
R2_ACCESS_KEY_ID=<r2-key>
R2_SECRET_ACCESS_KEY=<r2-secret>
R2_BUCKET_NAME=myathan-files
R2_PUBLIC_URL=https://files.myathan.com
IP_API_KEY=<optional-for-paid-tier>
NODE_ENV=production
PORT=3000
```

### External Services (not on VPS)
| Service | Purpose | Cost |
|---|---|---|
| **Cloudflare R2** | Firmware binaries + audio files (S3-compatible) | Free tier: 10GB storage, 10M reads/mo |
| **Cloudflare DNS** | DNS management + CDN for static assets (optional) | Free |
| **GitHub Actions** | CI/CD: build firmware, run tests, trigger Coolify deploy | Free for public repos |

### Monitoring & Maintenance
- **Coolify dashboard**: View container logs, resource usage, deployment history
- **PostgreSQL backups**: Daily automated via Coolify → R2
- **VPS monitoring**: Hostinger panel provides CPU/RAM/disk metrics
- **Uptime monitoring**: Add free UptimeRobot check on `api.myathan.com/health`

---

## ESP32-C3 Pin Mapping

```
ESP32-C3 SuperMini Pins:
  GPIO4  → DFPlayer TX (Software Serial RX)
  GPIO5  → DFPlayer RX (Software Serial TX)
  GPIO8  → WS2812B LED data (or plain LED)
  GPIO9  → Button (INPUT_PULLUP, active LOW)
  GPIO10 → SD card CS (for future SPI SD write, v2)
  GPIO6  → SD card MOSI (v2)
  GPIO7  → SD card MISO (v2)
  GPIO2  → SD card CLK (v2)
```

---

## Implementation Phases

### Phase 1 — Firmware Foundation (this repo)
**Goal**: Device boots, connects to WiFi, plays audio on schedule.
1. PlatformIO project setup targeting ESP32-C3 SuperMini
2. BLE WiFi provisioning (ESP-IDF WiFiProv) + WiFiManager captive portal fallback
3. NTP time sync with timezone support
4. LittleFS config manager (read/write/defaults)
5. DFPlayer Mini driver: play, stop, volume, track select
6. LED state machine (non-blocking patterns)
7. Prayer scheduler: check time every 30s, trigger audio + LED

### Phase 1.5 — Infrastructure Setup
**Goal**: VPS is running with Coolify, ready to host all services.
1. Provision Hostinger VPS (Ubuntu 22.04+, 4 vCPU / 8GB recommended)
2. Install Coolify: `curl -fsSL https://cdn.coollabs.io/coolify/install.sh | bash`
3. Configure DNS: A records for `api.myathan.com`, `app.myathan.com`, `admin.myathan.com` → VPS IP
4. Create PostgreSQL 16 resource in Coolify
5. Create Cloudflare R2 bucket (`myathan-files`) + generate API keys
6. Set up GitHub webhook integration in Coolify for auto-deploy
7. Configure daily PostgreSQL backup to R2
8. Verify: Coolify dashboard accessible, PostgreSQL running, DNS resolving, SSL certs provisioned

### Phase 2 — Backend + Connectivity
**Goal**: Device fetches timetable from cloud, config is synced.
1. Fastify + TypeScript + PostgreSQL project setup
2. `/timetable` endpoint: adhan-js + IP geolocation
3. Device registry + config sync endpoints
4. Firmware: HTTPS client → fetch timetable on boot → cache in LittleFS
5. Firmware: config polling every 5 min
6. Firmware: heartbeat (hourly) + stats batch (daily)
7. Offline fallback: use cached timetable if no internet

### Phase 3 — OTA System
**Goal**: Devices auto-update firmware.
1. Backend: releases table + R2 upload
2. Firmware: OTA manager (check → download → verify → flash → rollback)
3. GitHub Actions CI: build on tag → upload binary to backend

### Phase 4 — Mobile PWA
**Goal**: Users can set up and control device from phone.
1. React + Vite + Tailwind scaffold with PWA plugin
2. Web Bluetooth: BLE scan → WiFi provisioning
3. Auth: register/login
4. Device pairing (scan QR or enter ID)
5. Timetable view + location picker + calculation method selector
6. Per-prayer audio file selection from catalog
7. LED + volume config panel
8. Doaa configuration per prayer
9. Stats dashboard

### Phase 5 — Admin Panel
**Goal**: Fleet management dashboard for monitoring and controlling all devices.
1. React + Vite + Tailwind scaffold (desktop-first SPA)
2. Admin auth (login with admin JWT)
3. Device fleet table: sortable/filterable by status, version, location, last seen
4. Online/offline status indicator (based on heartbeat)
5. Device detail view: config, stats, heartbeat history, error log
6. Firmware release management: upload binary, view rollout progress
7. Push OTA to specific device(s)
8. Remote audio config: change athan/doaa per prayer per device
9. Bulk config push to multiple devices
10. Audio file catalog management (upload/delete MP3s)
11. Aggregate analytics dashboard (charts: plays/day, active devices, geo distribution)
12. Per-device statistics view
13. Alert management: offline devices, OTA failures, repeated errors

### Phase 6 — Polish & v2
1. Audio file OTA: ESP32 downloads MP3 from R2 → writes to SD via SPI
2. Manual timetable offset UI
3. Scheduled OTA rollouts (staged: 10% → 50% → 100%)
4. Email/webhook notifications for alerts
5. Admin audit log (who changed what config)
6. Multi-device support per user account

---

---

## Security Hardening

### Device Authentication & API Keys
- **Problem**: Storing API keys in plaintext config is a vulnerability
- **Solution**:
  - Device API key derived from **hardware MAC address + server-side salt**
  - Backend stores: `device_api_key = HMAC-SHA256(device_mac + BACKEND_SECRET)`
  - Device never stores API key in config.json; derives it on boot from own MAC
  - Backend validates request: `compute_key(request.mac) == request.provided_key`

### Local HTTP Authentication
- **Problem**: Unencrypted local HTTP on `myathan.local` allows LAN-based tampering
- **Solution**:
  - Add optional **basic auth** for local HTTP endpoints: `/status`, `/trigger`, `/config` (user-configurable PIN)
  - Endpoints are rate-limited to 60 req/min per device
  - WiFi credentials not exposed via local HTTP (only GET /status)
  - Sensitive writes (/trigger, /config) require auth token

### Cloud Communication
- **Backend rate limiting**: 60 requests/min per device ID
- **TLS certificate pinning** (optional, Phase 2): Pin Cloudflare + Let's Encrypt root CAs to ESP32 firmware
- **Heartbeat auth**: Include HMAC of deviceId + timestamp to prevent spoofing
- **Config versioning**: Backend tracks config version; device only accepts config from trusted source

### Offline Security
- **Cached timetable**: Encrypted at rest in LittleFS (optional, Phase 2)
- **Device resets to defaults** if offline >30 days (configurable)
- **No credential leakage**: WiFi password stored in LittleFS only; not logged or transmitted

---

## Connection State Machine

### Firmware Connection States

Device moves through these states based on WiFi + backend connectivity:

```
┌─────────────────────┐
│   DISCONNECTED      │  No WiFi, no cloud
├─────────────────────┤
│ Actions:            │
│ - BLE advertising   │
│ - LED: pulse        │
│ - Use cached times  │
└──────────┬──────────┘
           │ (WiFi found & connected)
           ▼
┌─────────────────────┐
│   WIFI_CONNECTED    │  WiFi OK, checking cloud
├─────────────────────┤
│ Actions:            │
│ - Sync NTP time     │
│ - Heartbeat to BE   │
│ - LED: slow blink   │
└──────────┬──────────┘
           │ (Backend responds)
           ▼
┌─────────────────────┐
│   CLOUD_SYNCED      │  Full sync: timetable + config ready
├─────────────────────┤
│ Actions:            │
│ - Poll config every 5min
│ - Heartbeat hourly  │
│ - LED: off (idle)   │
│ - Play athan at time│
└──────────┬──────────┘
           │ (Playing athan)
           ▼
┌─────────────────────┐
│   PLAYING           │  DFPlayer actively playing audio
├─────────────────────┤
│ Actions:            │
│ - LED: solid        │
│ - Cancel pre-athan  │
│ - Queue doaa after  │
└──────────┬──────────┘
           │ (Finished or stopped)
           ▼
    [back to CLOUD_SYNCED]
```

**WiFiManager + ESPAsyncWebServer Mutual Exclusion**:
- Both compete for port 80 and WiFi scan results
- Solution: 
  1. ESPAsyncWebServer starts once WiFi connected (CLOUD_SYNCED state)
  2. On WiFi loss, stop HTTP server → restart WiFiManager captive portal
  3. WiFiManager captive portal uses same port 80 (no conflict when server is stopped)
  4. Once WiFi reconnected, stop WiFiManager → restart ESPAsyncWebServer

---

## GPIO Pin Allocation & UART Configuration

### ESP32-C3 SuperMini Pin Map

| GPIO | Function | Purpose | Notes |
|---|---|---|---|
| **GPIO20** | UART0 RX | Serial/CDC logging | USB-CDC debug output (not user-configurable) |
| **GPIO21** | UART0 TX | Serial/CDC logging | USB-CDC debug output (not user-configurable) |
| **GPIO6** | UART1 TX | DFPlayer Mini command | Hardware UART, 9600 baud, must use Serial1 |
| **GPIO7** | UART1 RX | DFPlayer Mini response | Hardware UART, 9600 baud, must use Serial1 |
| **GPIO8** | GPIO | LED status indicator | PWM-capable for brightness, low=on (active low) |
| **GPIO9** | GPIO | Button (reset/trigger) | Pull-up enabled, debounced 50ms |
| **GPIO0–5, 10–19** | (available) | Future expansion | SPI for SD card (future OTA audio) |

**Critical**: 
- Use **Hardware Serial1** (GPIO6/7) for DFPlayer, NOT SoftwareSerial → avoids timing issues, supports 9600 baud reliably
- Do NOT use GPIO6/7 for other functions
- GPIO8/9 can be remapped in config but shown here are defaults

---

## Disaster Recovery & Infrastructure Resilience

### PostgreSQL Backup & Recovery

**Daily Backup Process**:
1. **Automated daily dump** (3AM UTC): `pg_dump myathan_db | gzip` → uploaded to Cloudflare R2 with timestamp
2. **Retention policy**: 30 days of backups (oldest auto-deleted)
3. **Offsite storage**: R2 is geographically distributed; redundant

**Recovery Procedure** (if database lost):
1. SSH into Hostinger VPS
2. Stop Coolify: `coolify system:stop`
3. Download latest backup: `aws s3 cp s3://myathan-files/backups/latest.sql.gz .`
4. Decompress: `gunzip latest.sql.gz`
5. Restore: `psql myathan_db < latest.sql`
6. Restart Coolify: `coolify system:start`
7. Verify: `curl https://api.myathan.com/health`

**Backup Verification** (weekly):
- Coolify automatically verifies restore-ability of backups
- Manual test on staging env: full restore from backup, run integration tests

### Application Recovery

**Firmware Binaries & Releases**:
- All firmware binaries stored in R2 (`myathan-files/releases/`)
- CI/CD on GitHub Actions builds binaries on tag push
- Backend downloads binary from R2 on admin OTA push
- Device downloads binary from backend on check (no direct R2 access)

**Device OTA Rollback**:
- ESP32-C3 has dual-partition OTA: app0 (current) + app1 (previous)
- If device doesn't heartbeat within **300s** post-update, automatically boot from app1 (previous partition)
- Admin can force rollback via backend: `POST /admin/releases/:id/rollback`

**Web App Recovery**:
- PWA is static (Vite build output)
- Stored on Coolify + CDN (future)
- Rollback: Coolify keeps 5 previous builds; revert with one click
- Admin panel same as PWA (separate SPA in same repo)

### Coolify Disaster Recovery

**Coolify Configuration Export**:
- Weekly automated export of docker-compose files + .env → R2
- Allows full reconstruction on new VPS if needed

**High Availability (optional, Phase 2)**:
- Current: Single VPS + PostgreSQL replica to staging server (manual)
- Future: Second VPS running hot standby; load balancer in front

---

## OTA Update Safety & Rollback Logic

### Update Flow

1. **Check Phase** (3AM daily or admin-triggered):
   - Device: `GET /api/v1/releases/latest?platform=esp32c3&current=0.1.0`
   - Backend returns: `{version: "0.2.0", url: "https://r2.../v0.2.0.bin", sha256: "abc123..."}`

2. **Pre-Update Validation**:
   - Check free heap: must have >200KB available (safety margin)
   - Check power: voltage >3.0V (not in low-battery state)
   - Check storage: must have >1MB free in flash (partition size check)

3. **Download Phase**:
   - `GET` binary from backend URL (or direct R2)
   - Stream to app1 partition (via `esp_ota_begin()`)
   - Compute running SHA256

4. **Verify Phase**:
   - Ensure downloaded SHA256 matches expected
   - If mismatch: abort, retry up to 3 times, then give up and log error

5. **Flash & Reboot**:
   - Finalize OTA (`esp_ota_end()`)
   - Set app1 as boot partition
   - Reboot

6. **Boot Phase** (app1):
   - If app1 boots successfully, mark it as permanent partition
   - Send heartbeat to backend: `POST /heartbeat {version: "0.2.0", status: "success"}`

7. **Rollback Trigger** (if app1 fails):
   - Boot watchdog timeout **300 seconds** before rollback
   - If heartbeat not sent within 300s post-update → device reboots into app0
   - On reboot to app0, firmware detects failure, sends: `POST /heartbeat {version: "0.1.0", status: "ota_failed", reason: "watchdog"}`
   - Backend sees rollback; admin notified via alert

### Device-Side Fallback Configuration

```cpp
#define OTA_ROLLBACK_TIMEOUT_MS 300000  // 5 minutes (configurable via config.json)
#define OTA_MAX_RETRIES 3
#define OTA_SAFE_HEAP_KB 200
```

---

## Config Versioning & Migrations

### Config Schema Evolution

**Current schema version**: `configVersion: 1`

When config schema changes (e.g., adding new field, changing structure):
1. Increment `CONFIG_VERSION` in `src/config/defaults.h`
2. Add migration function in `ConfigManager.cpp`:
   ```cpp
   bool ConfigManager::_migratev1_to_v2() {
       // Transform old config to new schema
       // E.g., rename "audioTrack" → "audio.defaultTrack"
       return save();
   }
   ```
3. In `load()`, check version and call migration if needed
4. Backend also tracks config version, can intelligently push configs to older devices

### Example: Adding a new field

**Old config.json**:
```json
{ "deviceId": "myathan-123", "volume": 20 }
```

**New config.json** (v2):
```json
{ "deviceId": "myathan-123", "configVersion": 2, "audio": {"volume": 20}, "network": {"timeout": 30} }
```

**Migration**:
```cpp
void ConfigManager::_migratev1_to_v2() {
    if (_doc["configVersion"] < 2) {
        int oldVolume = _doc["volume"] | 20;
        _doc.clear();
        _doc["configVersion"] = 2;
        _doc["audio"]["volume"] = oldVolume;
        _doc["network"]["timeout"] = 30;
        save();
    }
}
```

---

## Verification Plan

### Firmware
- `pio run` — compiles successfully for ESP32-C3
- `pio test` — unit tests pass (config parsing, prayer time matching, scheduler logic)
- `pio run -t upload` — flashes to ESP32-C3 SuperMini
- Serial monitor shows: WiFi connected, NTP synced, timetable loaded
- Phone BLE scan sees "MyAthan-XXXX", can send WiFi credentials
- DFPlayer plays correct track at prayer time
- LED blinks before athan, solid during, off after
- `POST /trigger` on `myathan.local` triggers manual athan
- OTA: flash old version → device auto-updates to new version overnight

### Infrastructure
- Coolify dashboard accessible at VPS_IP:8000
- PostgreSQL container running and accepting connections
- DNS resolves: `api.myathan.com`, `app.myathan.com`, `admin.myathan.com`
- SSL certificates provisioned (Let's Encrypt via Traefik)
- R2 bucket accessible with API keys
- GitHub webhook triggers auto-deploy on push

### Backend
- `npm test` — adhan-js timetable calculation matches reference
- `npm run dev` + curl `/api/v1/timetable` → correct times for test coordinates
- Device registration → heartbeat → config pull cycle works end-to-end
- Deploy to Coolify → `https://api.myathan.com/health` returns 200
- HTTPS endpoints accessible from ESP32-C3 device

### Mobile App
- `npm run dev` → opens on mobile Chrome
- Web Bluetooth pairs with ESP32-C3 → WiFi provisioned
- Change location → device picks up new timetable on next poll
- Install as PWA → works offline (cached shell)
- Audio file upload → appears in catalog → assignable to prayer

### Admin Panel
- `npm run dev` → opens in desktop browser
- Admin login → device fleet table loads with all registered devices
- Online/offline indicators match actual heartbeat status
- Click device → detail view shows correct config, stats, and heartbeat history
- Upload firmware binary → appears in releases list with rollout stats
- Push OTA to device → device updates on next check cycle
- Change audio config for device → device picks up new tracks on next config poll
- Bulk config push → multiple devices receive updated config
- Upload/delete audio files → catalog reflects changes
- Analytics dashboard → charts render correctly with real device data
- Alert for offline device appears after heartbeat gap >2 hours
