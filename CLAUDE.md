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

## Claude Model & Configuration Rules

### Decision Matrix — Pick the cheapest option that gets the job done

| Task | Model | Version | Effort | Thinking | 1M | Cost Tier |
|------|-------|---------|--------|----------|----|-----------|
| Prayer calculation algorithms, high-latitude math | **Opus** | 4.6 | max | ON | no | $$$$ |
| Architecture changes, cross-repo schema updates | **Opus** | 4.6 | high | ON | no | $$$ |
| Debugging multi-module issues (scheduler+audio+prayer) | **Opus** | 4.6 | high | ON | no | $$$ |
| Feature implementation, new driver/module | **Sonnet** | 4.6 | high | OFF | no | $$ |
| Bug fixes, config changes | **Sonnet** | 4.6 | med | OFF | no | $$ |
| Code review with ESP32 safety checklist | **Sonnet** | 4.6 | high | OFF | no | $$ |
| Single-file edits, header updates | **Sonnet** | 4.6 | med | OFF | no | $$ |
| Build/flash/test commands | **Haiku** | 4.5 | low | OFF | no | $ |
| Simple config.json tweaks | **Haiku** | 4.5 | low | OFF | no | $ |
| Git operations, file lookups | **Haiku** | 4.5 | low | OFF | no | $ |

### Automation Rules

**Version selection:**
- Always use **4.6** for Opus and Sonnet — latest generation, strictly better
- Haiku is **4.5** only (latest available)

**When to enable thinking:**
- ON: Prayer math (trig, angle-of-sun), high-latitude edge cases, multi-module debugging, config migration design
- OFF: Everything else — embedded code follows clear patterns, thinking adds cost without proportional benefit

**When to use 1M context:**
- Almost never for this repo — firmware is ~20 source files, well under standard context limits
- Only if reading ARCHITECTURE.md (36KB) + multiple source files + tests simultaneously
- Never for build/flash/test, single-file edits, or standard feature work

**Effort level guide:**
- `max`: Only for prayer calculation math — wrong results mean wrong prayer times (high stakes)
- `high`: Multi-file changes, code review (ESP32 safety matters), new modules
- `med`: Bug fixes, config changes, single-file feature work
- `low`: CLI execution (build/flash/test), simple lookups, git commands

**Cost awareness (relative per task):**
- Haiku 4.5 low/no-think = **1x baseline** (~$0.25/M in, $1.25/M out)
- Sonnet 4.6 med/no-think = **~12x** (~$3/M in, $15/M out)
- Opus 4.6 high/thinking = **~100x** (~$15/M in, $75/M out + thinking tokens)
- 1M context adds premium on top — almost never needed for this repo
- **Default to Sonnet 4.6 med/no-think** unless the task clearly needs more or less

## Cost & Performance Optimization

**Prompt caching (automatic):**
- Claude caches repeated context (CLAUDE.md, skill definitions) at 90% discount
- Keep CLAUDE.md stable — frequent edits invalidate the cache
- Put rarely-changing content (tech stack, architecture) at the top, volatile content (branch names) at the bottom

**Context management:**
- Claude Code auto-compresses conversation history approaching limits — no action needed
- For long sessions: start new sessions rather than accumulating stale context
- Firmware source files are small (<500 lines each) — always readable in standard context

**Subagent cost delegation:**
- Use `subagent_type: "Explore"` with Haiku/Sonnet for codebase searches before invoking Opus
- Delegate independent research tasks to parallel subagents — faster AND cheaper than serial Opus calls
- Never use Opus for file discovery — use Grep/Glob tools directly (zero LLM cost)

**Token-saving patterns:**
- Skills with `disable-model-invocation: true` use zero reasoning tokens — keep this on pure CLI skills (/build, /flash)
- Auto-allowed tools in settings.json skip permission prompts — saves round-trip tokens
- Tables and lists in CLAUDE.md are 30-50% more token-efficient than prose paragraphs
- Reference files by path instead of describing them — Claude reads the file instead of guessing

**Avoid these cost traps:**
- Don't enable thinking mode for simple edits — adds ~2-5x output tokens with no quality gain
- Don't use 1M context — this repo has ~20 source files, well under standard limits
- Don't re-read files already in conversation context — Claude remembers what it read
- Don't ask Opus to run build/flash/test — Haiku executes PlatformIO commands identically at 60x less cost

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
