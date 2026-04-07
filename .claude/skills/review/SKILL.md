---
name: review
description: Review firmware code changes for quality, safety, and ESP32 best practices. Use before committing or merging.
allowed-tools: Bash Read Grep Glob
---

# Firmware Code Review

Review current changes against MyAthan firmware standards.

## Review Checklist

### ESP32-C3 Constraints
- [ ] Total RAM usage under 300KB (400KB total, need headroom)
- [ ] No blocking delays in loop() (except `delay(10)` yield)
- [ ] No SoftwareSerial (use Hardware UART1 for DFPlayer)
- [ ] GPIO assignments don't conflict (6/7=DFPlayer, 8=LED, 9=Button)
- [ ] LittleFS writes are minimized (flash wear concern)

### Prayer Calculation Safety
- [ ] Prayer times are always in order: Fajr < Sunrise < Dhuhr < Asr < Maghrib < Isha
- [ ] High-latitude adjustments applied when |latitude| > 48°
- [ ] All 7 calculation methods produce valid output
- [ ] ASR Standard vs Hanafi difference is 20-90 minutes

### Config Schema
- [ ] New config fields have defaults in `_applyDefaults()`
- [ ] New fields are added in `_migrateIfNeeded()` for v1→v2
- [ ] CONFIG_MAX_SIZE (8192) is sufficient for new fields
- [ ] Getters return safe defaults for missing fields

### Audio Safety
- [ ] Volume always constrained 0-30
- [ ] Non-blocking playback detection (no while loops waiting for DFPlayer)
- [ ] Preview auto-stops after PREVIEW_DURATION_MS

### OTA Update Safety
- [ ] Config backed up to NVS before applyUpdate() starts download
- [ ] Config restored from NVS backup on boot after version change
- [ ] HARDWARE_TYPE defined via build_flags (not hardcoded in source)
- [ ] OTA check respects user preferences (checkHour, checkFrequency, autoUpdateEnabled)
- [ ] Force commands from admin override user autoUpdate preference
- [ ] SHA256/checksum verified after download (Update library or manual)
- [ ] OTA download timeout is reasonable (30s default)
- [ ] Heap check before OTA (>50KB free required)
- [ ] Binary size check before OTA (<=1.5MB)
- [ ] Rollback: esp_ota_mark_app_valid_cancel_rollback() called on successful boot
- [ ] justUpdated flag reported in heartbeat after OTA
- [ ] No OTA during active prayer playback (check scheduler state)

### General
- [ ] No `String` concatenation in tight loops (heap fragmentation)
- [ ] Serial.printf used instead of String concatenation for logging
- [ ] Watchdog-safe: no operations blocking for >5 seconds

## Steps
1. Run `git diff` to see all changes
2. Read each modified file
3. Apply the checklist above
4. Report findings with file:line references
