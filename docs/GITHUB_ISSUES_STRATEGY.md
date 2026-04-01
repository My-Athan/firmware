# GitHub Issues & Repository Distribution Strategy

## Overview

All 7 epic issues (#2-#8) are currently on the `My-Athan/firmware` repository. However, many stories should live on their respective repos (`My-Athan/backend`, `My-Athan/web`) for better organization and access control.

**Why**: 
- Firmware developers care about firmware stories (F1.1-F1.10, O3.3-O3.4, S1-S2, E1-E2, T1-T2)
- Backend developers own backend stories (B2.1-B2.5, O3.1-O3.2, S3-S5, E3, B2.6-B2.10 are firmware but depend on backend)
- Web developers own app/admin stories (M4.1-M4.11, A5.1-A5.13, I1-I3)

---

## Current State (All on firmware repo)

### Existing GitHub Issues on My-Athan/firmware

| Issue | Epic | Repo | Action |
|---|---|---|---|
| #2 | Phase 1: Firmware Foundation | firmware | **KEEP** (F1.1-F1.10 are firmware-only) |
| #3 | Phase 1.5: Infrastructure Setup | firmware | **MOVE to backend** (Coolify, PostgreSQL, monitoring are backend infra) |
| #4 | Phase 2: Backend + Connectivity | firmware | **SPLIT**: Keep firmware parts (B2.6-B2.10 on firmware), move server parts to backend |
| #5 | Phase 3: OTA System | firmware | **SPLIT**: Keep firmware OTA (O3.3-O3.4) on firmware; move backend OTA to backend (#3.1-O3.2) |
| #6 | Phase 4: Mobile PWA | firmware | **MOVE to web** (M4.1-M4.11 are entirely web repo) |
| #7 | Phase 5: Admin Panel | firmware | **MOVE to web** (A5.1-A5.13 are entirely web repo) |
| #8 | Phase 6: Polish & v2 | firmware | **SPLIT**: Keep firmware parts (P6.1 audio OTA) on firmware; move app parts to web |

---

## Recommended Distribution

### My-Athan/firmware (keep/modify)

**Epic #2: Firmware Foundation** (keep as-is)
```
F1.1  - PlatformIO setup
F1.2  - BLE WiFi provisioning (firmware-only)
F1.3  - WiFiManager fallback (firmware-only)
F1.4  - NTP time sync (firmware-only)
F1.5  - LittleFS config manager (firmware-only)
F1.6  - DFPlayer driver (firmware-only)
F1.7  - LED state machine (firmware-only)
F1.8  - Prayer scheduler (firmware-only)
F1.9  - Button handler (firmware-only)
F1.10 - Local HTTP server (firmware-only)
```

**Epic #3 (NEW): Firmware Infrastructure & OTA**
```
[Move from firmware #3 and #5, combined]
I1.1-I1.9  - Infrastructure setup (PostgreSQL, Coolify, backups, DNS)
O3.3 - Firmware OTA manager
O3.4 - Firmware OTA rollback
P6.1 - Audio file OTA to SD (Phase 6)
T1-T2 - Firmware testing (unit + E2E)
E1-E2 - Firmware error codes & logging
S1-S2 - Firmware security hardening (API keys, local auth)
O1-O3 - Offline fallback & config versioning
```

**New Epic: Security & Resilience (firmware perspective)**
```
All S1-S4, E1-E4, O1-O3 stories structured as firmware/backend splits
```

---

### My-Athan/backend (create new issues)

**Epic #3 (NEW): Backend Infrastructure & OTA**
```
I1.1-I1.9  - Infrastructure: Coolify, PostgreSQL, R2, DNS, monitoring, backups
O3.1 - Backend releases table + R2 upload
O3.2 - Backend releases API endpoint
B2.1 - Fastify + TS + PostgreSQL setup
```

**Epic #4 (NEW): Backend API & Connectivity**
```
B2.2 - Timetable endpoint (prayer time calculation)
B2.3 - Device registry API
B2.4 - Config sync endpoints
B2.5 - Auth system (API key + JWT + RBAC)
E3 - Error code definitions & documentation
S3 - Rate limiting middleware
S4 - TLS cert pinning support (optional)
S5 - GDPR compliance (data export, deletion)
```

**Epic #5 (NEW): OTA & Analytics**
```
[Subset of firmware #3, #5, #6]
O3.1 - Releases table & R2 upload
O3.2 - Releases API
B2.9 - Statistics batch collection
Backend analytics aggregation (for admin dashboard)
```

**Epic #6 (NEW): Testing**
```
T3 - Backend API integration tests
Cron job testing (OTA checks, backup schedule)
```

---

### My-Athan/web (create new issues)

**Epic #4 (NEW): Mobile PWA**
```
M4.1 - React + Vite + Tailwind PWA scaffold
M4.2 - Web Bluetooth WiFi provisioning
M4.3 - Auth (register/login with JWT)
M4.4 - Device pairing
M4.5 - Timetable view
M4.6 - Location picker + calculation method
M4.7 - Per-prayer audio selection
M4.8 - Doaa configuration
M4.9 - LED + volume config
M4.10 - Manual timetable override
M4.11 - Statistics dashboard
I1-I2 - PWA i18n (Arabic + English)
T4 - PWA UI component tests
E4 - PWA error boundary UI
```

**Epic #5 (NEW): Admin Panel**
```
A5.1 - React + Vite + MUI scaffold
A5.2 - Admin auth (JWT role check)
A5.3 - Device fleet table (MUI DataGrid)
A5.4 - Online/offline status indicators
A5.5 - Device detail view (config, stats, history)
A5.6 - Firmware release management
A5.7 - Push OTA to device(s)
A5.8 - Remote audio config
A5.9 - Bulk config push
A5.10 - Audio file catalog management
A5.11 - Aggregate analytics dashboard
A5.12 - Per-device statistics
A5.13 - Alert management
I3 - Admin Panel i18n
T4 (part 2) - Admin component tests
```

**Epic #6 (NEW): Polish & v2**
```
P6.2 - Staged OTA rollouts (backend + frontend)
P6.3 - Email/webhook alerts
P6.4 - Admin audit log
P6.5 - Multi-device per user
```

---

## Cross-Repo Dependencies

### Firmware → Backend
- F1.2 (BLE provisioning) sends WiFi creds; device registers with backend in B2.3
- F1.8 (Prayer scheduler) depends on timetable from B2.2
- B2.6-B2.10 (firmware client code) depends on backend APIs B2.2-B2.5

### Backend → Firmware
- O3.2 (releases API) serves firmware binaries to devices
- B2.4 (config sync) pushes config changes to devices

### Web → Backend
- M4.3 (user auth) uses backend JWT
- M4.7 (audio selection) lists catalog from backend
- A5 (admin) uses backend analytics + device management APIs

### Web → Firmware
- M4.2 (BLE provisioning) pairs with firmware; send WiFi creds
- M4.5-M4.8 (device config) writes to firmware via backend config sync

---

## GitHub Project #1 Linking

Once issues are redistributed:

1. **Create GitHub Project #1 at org level** (https://github.com/orgs/My-Athan/projects/1)
2. **Link all issues** from all 3 repos (firmware, backend, web)
3. **Status board**: Backlog → Todo → In Progress → Review → Done
4. **Milestones**: Phase 1 (firmware foundation), Phase 1.5 (infra), Phase 2 (backend), Phase 3 (OTA), Phase 4 (PWA), Phase 5 (admin), Phase 6 (polish)

---

## Manual Issue Creation Steps

Since GitHub MCP is unavailable, create issues manually via GitHub web UI:

### For My-Athan/firmware (existing)
- Issues #2-#8 already exist
- Update descriptions to split stories as documented above
- Link child issues using GitHub issue relationships

### For My-Athan/backend (new)
1. Create Epic: "Phase 1.5: Infrastructure Setup" (description + Infrastructure story checklist)
2. Create Epic: "Phase 2: Backend + Connectivity" (B2.1-B2.5)
3. Create Epic: "Phase 3: OTA System" (O3.1-O3.2)
4. Create child issues for each story

### For My-Athan/web (new)
1. Create Epic: "Phase 4: Mobile PWA" (M4.1-M4.11 + i18n)
2. Create Epic: "Phase 5: Admin Panel" (A5.1-A5.13)
3. Create child issues

---

## Benefits of Redistribution

✅ **Organization**: Developers see only their repo's issues  
✅ **Ownership**: Backend team owns backend features; web team owns UI  
✅ **Planning**: Easier to estimate and schedule per-team  
✅ **CI/CD**: Each repo has independent CI pipeline  
✅ **Naming**: Issue #2 in firmware, #1 in backend are both "Phase 1"  

---

## Rollback Plan

If needed, all issues can be recreated on firmware repo with labels:
- `repo:firmware`, `repo:backend`, `repo:web`
- `epic:phase1`, `epic:phase2`, etc.

But distribution to separate repos is **strongly recommended** for multi-team coordination.
