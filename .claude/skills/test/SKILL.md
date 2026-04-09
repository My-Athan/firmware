---
name: test
description: Run firmware unit tests, analyze failures, and fix issues. Use when testing prayer calculator, Hijri calendar, or other firmware components.
allowed-tools: Bash Read Grep Glob Edit
argument-hint: "[test_name]"
---

# Run Firmware Tests

Execute PlatformIO unit tests for the MyAthan firmware, analyze failures, and fix them.

## Steps

1. If argument provided, run specific test: `pio test -e esp32c3 -f $ARGUMENTS`
2. If no argument, run all tests: `pio test -e esp32c3`
3. Analyze results:
   - Report pass/fail counts
   - For failures:
     1. Read the failing test file in `test/`
     2. Read the corresponding source code in `src/`
     3. Trace the calculation logic to find the discrepancy
     4. Fix the source code or test expectation as appropriate
     5. Re-run the specific failing test to verify
   - Suggest fixes for any failing tests

## Available Test Suites
- `test_prayer_calculator` — 10 tests: prayer times for multiple cities, all 7 methods, high-latitude, ASR Standard vs Hanafi
  - Source: `src/prayer/PrayerCalculator.h`, `src/prayer/PrayerCalculator.cpp`
- `test_hijri_calendar` — 18 tests: Gregorian→Hijri conversion, Ramadan detection, 7 holidays, leap years, adjustment
  - Source: `src/prayer/HijriCalendar.h`, `src/prayer/HijriCalendar.cpp`

## Test References
- Prayer times can be verified against https://aladhan.com/prayer-times-api
- Hijri dates can be verified against https://www.islamicfinder.org/islamic-calendar/

## On Failure
1. Read the test output to identify which assertion failed
2. Compare expected vs actual values
3. Search the source for the calculation that produces the wrong value
4. Fix the root cause, not just the test expectation
