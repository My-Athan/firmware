# MyAthan — Implementation Plan & Project Backlog

## Context
We have a fully approved architecture plan (in `polished-humming-conway.md`) for the MyAthan ecosystem: ESP32-C3 firmware + DFPlayer Mini, backend API (Fastify/TS), and a web monorepo (mobile PWA + admin panel). 3 repos total.

GitHub issue creation is blocked (403 — integration lacks Issues write permission on the `My-Athan` org). This plan serves as the **complete project backlog** with all epics, stories, and tasks, ready to be committed to the repo as `PROJECT.md` and later imported into GitHub Projects or any tracker.

## What We're Doing
1. Commit the full architecture plan + project backlog as `PROJECT.md` to `My-Athan/firmware`
2. Commit the initial PlatformIO project scaffolding (Phase 1 foundation)
3. Push to branch `claude/myathan-device-setup-9Y3ml`

---

## Files to Create/Modify

### 1. `PROJECT.md` — Full project backlog (new file)
Contains all 6 epics with stories and acceptance criteria, structured for easy import into GitHub Projects.

### 2. `platformio.ini` — PlatformIO config (new file)
ESP32-C3 SuperMini target with required libraries.

### 3. `include/version.h` — Build version header (new file)

### 4. `src/main.cpp` — Entry point skeleton (new file)

### 5. `src/config/ConfigManager.h` / `.cpp` — LittleFS config (new files)

### 6. `src/config/defaults.h` — Default config values (new file)

### 7. `data/config.json` — Default LittleFS config (new file)

### 8. `.gitignore` — PlatformIO gitignore (new file)

---

## PROJECT.md Content — Full Backlog

### Epic 1: Firmware Foundation (Phase 1)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| F1.1 | Task | PlatformIO project setup for ESP32-C3 SuperMini | `pio run` compiles clean; platformio.ini targets esp32-c3-devkitm-1 |
| F1.2 | Story | BLE WiFi provisioning | Phone BLE scan sees "MyAthan-XXXX"; credentials sent over BLE; device connects to WiFi |
| F1.3 | Story | WiFiManager captive portal fallback | On BLE failure/timeout, device starts AP "MyAthan-Setup"; captive portal serves WiFi form |
| F1.4 | Story | NTP time sync with timezone | Device syncs time on WiFi connect; timezone configurable; time survives brief disconnects |
| F1.5 | Story | LittleFS config manager | Read/write JSON config; defaults on first boot; persists across reboots |
| F1.6 | Story | DFPlayer Mini audio driver | Play/stop/volume/track select via UART; handles busy state; error detection |
| F1.7 | Story | LED state machine | Non-blocking patterns: idle(off), pre-athan(slow_blink), playing(solid), error(fast_blink), no-wifi(pulse) |
| F1.8 | Story | Prayer scheduler | Check time every 30s; trigger correct track at prayer time; handle doaa delay; skip if already played |
| F1.9 | Story | Button handler | Short press = manual athan trigger; long press 5s = WiFi reset; debounced, non-blocking |
| F1.10 | Story | Local HTTP server | ESPAsyncWebServer on port 80; mDNS `myathan.local`; endpoints: /status, /trigger, /volume, /config |

### Epic 1.5: Infrastructure Setup (Phase 1.5)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| I1.1 | Task | Provision Hostinger VPS | Ubuntu 22.04+ VPS running, SSH accessible, firewall configured (22, 80, 443, 8000) |
| I1.2 | Task | Install Coolify | Coolify dashboard accessible at VPS_IP:8000; Traefik reverse proxy running |
| I1.3 | Task | Configure DNS | A records for `api.myathan.com`, `app.myathan.com`, `admin.myathan.com` → VPS IP; DNS resolving |
| I1.4 | Task | Set up PostgreSQL 16 | Coolify-managed PostgreSQL container running; internal network only; credentials stored |
| I1.5 | Task | Set up Cloudflare R2 bucket | `myathan-files` bucket created; API keys generated; CORS configured for device + web access |
| I1.6 | Task | Configure Coolify auto-deploy | GitHub webhook connected; push to main triggers rebuild + zero-downtime deploy |
| I1.7 | Task | Configure backups | Daily PostgreSQL dump → R2; tested restore procedure |
| I1.8 | Task | SSL + domain verification | Let's Encrypt certs provisioned for all subdomains; HTTPS working |
| I1.9 | Task | Set up monitoring | UptimeRobot on `api.myathan.com/health`; Coolify container metrics visible |

### Epic 2: Backend + Connectivity (Phase 2)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| B2.1 | Task | Fastify + TypeScript + PostgreSQL project setup | `npm run dev` starts server; DB migrations run; health check endpoint responds |
| B2.2 | Story | Timetable endpoint | `GET /api/v1/timetable` returns correct prayer times for IP; `?lat=&lon=&method=` override works |
| B2.3 | Story | Device registry | Device registers with UUID on first connect; stored in DB with metadata |
| B2.4 | Story | Config sync endpoints | `GET /config/:id` returns device config; `POST /config/:id` updates it; version tracking |
| B2.5 | Story | Auth system | API key auth for devices; JWT auth for app/admin users; RBAC middleware (user vs admin) |
| B2.6 | Story | Firmware: HTTPS backend client | ESP32 fetches timetable over HTTPS on boot; retries on failure; caches in LittleFS |
| B2.7 | Story | Firmware: config polling | Device polls `GET /config/:id` every 5 min; applies changes (volume, audio, LED) |
| B2.8 | Story | Firmware: heartbeat | Hourly `POST /heartbeat` with deviceId, version, freeHeap, uptime |
| B2.9 | Story | Firmware: statistics batch upload | Daily `POST /stats` with play counts per prayer, errors, uptime |
| B2.10 | Story | Firmware: offline fallback | Cache last 7 days timetable; work without internet using cached data |

### Epic 3: OTA System (Phase 3)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| O3.1 | Story | Backend: releases table + R2 upload | Admin can upload firmware binary; stored in R2; metadata in DB (version, sha256, size) |
| O3.2 | Story | Backend: releases API | `GET /releases/latest?platform=esp32c3` returns latest version + download URL |
| O3.3 | Story | Firmware: OTA manager | Check at 3AM; download if newer; verify SHA256; flash; reboot |
| O3.4 | Story | Firmware: OTA rollback | If device doesn't heartbeat within 120s post-update, boot previous partition |
| O3.5 | Task | GitHub Actions CI pipeline | Build firmware binary on tag push; upload to backend releases endpoint |

### Epic 4: Mobile PWA (Phase 4)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| M4.1 | Task | React + Vite + Tailwind + PWA scaffold | `npm run dev` serves app; installable as PWA; offline shell works |
| M4.2 | Story | Web Bluetooth WiFi provisioning | BLE scan finds "MyAthan-XXXX"; send SSID + password; device connects |
| M4.3 | Story | Auth (register/login) | User registers with email/password; login returns JWT; token persists |
| M4.4 | Story | Device pairing | Enter device ID or scan QR; link device to user account via backend |
| M4.5 | Story | Timetable view | Display today's prayer times; auto-refresh daily; show next prayer countdown |
| M4.6 | Story | Location picker | City search or map pin; select calculation method (ISNA, MWL, etc.); updates timetable |
| M4.7 | Story | Per-prayer audio selection | Browse audio catalog; assign different athan file to each of 5 prayers |
| M4.8 | Story | Doaa configuration | Enable/disable doaa per prayer; select doaa file; set delay (1-10 min) |
| M4.9 | Story | LED + volume config | Toggle LED on/off; set pre-athan lead time; adjust volume slider |
| M4.10 | Story | Manual timetable override | Per-prayer ±30 min offset slider; saved to device config |
| M4.11 | Story | Statistics dashboard | Plays per prayer chart; device uptime; last seen; connection status |

### Epic 5: Admin Panel (Phase 5)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| A5.1 | Task | React + Vite + MUI scaffold | `npm run dev` serves admin SPA; MUI theme configured |
| A5.2 | Story | Admin auth | Admin login with JWT; admin role check; redirect if unauthorized |
| A5.3 | Story | Device fleet table | MUI DataGrid: columns for ID, status, version, location, last heartbeat, owner; sortable + filterable |
| A5.4 | Story | Online/offline status | Green dot = heartbeat within 2h; red dot = stale; yellow = recently offline |
| A5.5 | Story | Device detail view | Click row → full config, timetable, play history, error log, uptime chart |
| A5.6 | Story | Firmware release management | Upload binary + release notes; see rollout status (updated/pending/failed counts) |
| A5.7 | Story | Push OTA to device(s) | Select device(s) → flag for immediate OTA on next poll |
| A5.8 | Story | Remote audio config | Select device → change athan track per prayer + doaa → pushed via config sync |
| A5.9 | Story | Bulk config push | Multi-select devices → apply same config change to all |
| A5.10 | Story | Audio file catalog management | Upload/delete MP3s to R2; manage catalog (name, type, duration, preview) |
| A5.11 | Story | Aggregate analytics dashboard | Charts: total devices, active/inactive, plays/day, geographic distribution, error rates |
| A5.12 | Story | Per-device statistics | Daily play count per prayer, missed athans, connectivity gaps, error timeline |
| A5.13 | Story | Alert management | List alerts (offline >24h, OTA failure, errors); acknowledge/dismiss |

### Epic 6: Polish & v2 (Phase 6)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| P6.1 | Story | Audio file OTA to SD card | ESP32 downloads MP3 from R2 via SPI → writes to SD card; DFPlayer sees new tracks |
| P6.2 | Story | Staged OTA rollouts | Release to 10% → 50% → 100%; rollback if error rate spikes |
| P6.3 | Story | Email/webhook alert notifications | Configure email or webhook for critical alerts; fires automatically |
| P6.4 | Story | Admin audit log | Log admin actions (config changes, OTA pushes) with user + timestamp |
| P6.5 | Story | Multi-device per user | User can pair and manage multiple MyAthan devices from one account |

---

## Immediate Action: Phase 1 Scaffolding

Since we're in the `firmware` repo, the first concrete deliverable is the PlatformIO project scaffold for Phase 1. This includes:

1. **`platformio.ini`** — ESP32-C3 target + library dependencies
2. **`include/version.h`** — Build version constant
3. **`src/main.cpp`** — Entry point with setup/loop skeleton
4. **`src/config/ConfigManager.h/.cpp`** — LittleFS config read/write
5. **`src/config/defaults.h`** — Default config values
6. **`data/config.json`** — Default LittleFS filesystem config
7. **`.gitignore`** — PlatformIO ignores
8. **`PROJECT.md`** — Full backlog (from this plan)

---

## Additional Stories — Missing Scope (Post-Review)

These stories were identified during code review as critical missing scope:

### Testing Strategy (Cross-repo)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| T1 | Task | Firmware: Unit test framework (Unity) | PlatformIO test suite configured; example config parsing test passes |
| T2 | Story | Firmware: Prayer scheduler unit tests | Test cases: exact match, pre-athan, doaa delay, skip repeated, offline fallback |
| T3 | Story | Backend: API integration tests (Vitest) | Test: timetable endpoint, device registry, config sync, OTA release flow |
| T4 | Story | Web: UI component tests (Testing Library) | Test: BLE provisioning flow, device list, config form, admin data grid |
| T5 | Story | End-to-end test (E2E) | Selenium/Cypress: setup device → change location → verify athan plays at correct time |

### Error Codes & Diagnostics
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| E1 | Task | Firmware: Error code enum | Define 20+ error codes (WiFi failed, DFPlayer busy, OTA fail, etc.) |
| E2 | Story | Firmware: Error logging & recovery | Log errors with code; retry logic; heartbeat includes last error |
| E3 | Story | Backend: Error code documentation | API error codes documented; frontend can map to user messages |
| E4 | Story | Web: Error boundary UI | Display user-friendly error messages; retry buttons for transient failures |

### Security Hardening
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| S1 | Story | Firmware: Device API key derivation | API key = HMAC-SHA256(MAC + backend_secret); not stored in config |
| S2 | Story | Firmware: Local HTTP basic auth (optional) | PIN-based auth for /trigger, /config endpoints |
| S3 | Story | Backend: Rate limiting | 60 req/min per device ID; return 429 on limit |
| S4 | Story | Backend: TLS cert pinning (optional) | Pin Cloudflare + Let's Encrypt root CAs in firmware |
| S5 | Story | Backend: GDPR compliance | User data export endpoint; device deletion cascades |

### First-Boot Offline Fallback
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| O1 | Story | Firmware: Embedded default timetable | Hardcode Mecca prayer times (or nearest city) in firmware binary |
| O2 | Story | Firmware: Offline scheduler | On first boot with no internet, use embedded timetable for 7 days |
| O3 | Story | Backend: Config versioning & migrations | Track config schema version; auto-migrate on load |

### Internationalization (i18n)
| ID | Type | Story | Acceptance Criteria |
|---|---|---|---|
| I1 | Task | Web PWA: i18n setup (react-i18next) | String catalogs for English + Arabic (MSA) |
| I2 | Story | Web PWA: Prayer name i18n | Fajr/Dhuhr/Asr/Maghrib/Isha in Arabic + English |
| I3 | Story | Admin Panel: i18n setup | Dashboard + admin pages in English + Arabic |

---

## Verification
- `pio run` compiles successfully targeting ESP32-C3
- Project structure matches the plan
- `docs/ARCHITECTURE.md` contains security, disaster recovery, connection state machine, GPIO table sections
- `docs/PROJECT.md` contains all 6 epics + ~60 stories across firmware, backend, web, testing, security, i18n
- `README.md` is comprehensive with pin mapping, config schema, build instructions
- Changes committed and pushed to feature branch and main branch
