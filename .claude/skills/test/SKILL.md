---
name: test
description: Run firmware unit tests using PlatformIO. Use when testing prayer calculator, Hijri calendar, or other firmware components.
disable-model-invocation: true
allowed-tools: Bash Read
argument-hint: "[test_name]"
---

# Run Firmware Tests

Execute PlatformIO unit tests for the MyAthan firmware.

## Steps

1. If argument provided, run specific test: `pio test -e esp32c3 -f $ARGUMENTS`
2. If no argument, run all tests: `pio test -e esp32c3`
3. Analyze results:
   - Report pass/fail counts
   - For failures: read the test file and source code, diagnose the issue
   - Suggest fixes for any failing tests

## Available Test Suites
- `test_prayer_calculator` — 10 tests: prayer times for multiple cities, all 7 methods, high-latitude, ASR Standard vs Hanafi
- `test_hijri_calendar` — 18 tests: Gregorian→Hijri conversion, Ramadan detection, 7 holidays, leap years, adjustment

## Test References
- Prayer times can be verified against https://aladhan.com/prayer-times-api
- Hijri dates can be verified against https://www.islamicfinder.org/islamic-calendar/
