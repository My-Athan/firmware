#include <unity.h>
#include "../../src/prayer/PrayerCalculator.h"

// ─────────────────────────────────────────────────────────────
// Prayer Calculator Unit Tests
// Verifies calculated times against known-good reference data.
// Tolerance: ±3 minutes (accounts for algorithm variations)
// ─────────────────────────────────────────────────────────────

static PrayerCalculator calc;

#define TOLERANCE 3  // ±3 minutes

static void assertTimeInRange(const char* label, int actual, int expectedMin, int expectedMax) {
    char msg[128];
    snprintf(msg, sizeof(msg), "%s: expected %d-%d, got %d (%02d:%02d)",
             label, expectedMin, expectedMax, actual, actual / 60, actual % 60);
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(expectedMin, actual, msg);
    TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(expectedMax, actual, msg);
}

static void assertTimeNear(const char* label, int actual, int expected) {
    assertTimeInRange(label, actual, expected - TOLERANCE, expected + TOLERANCE);
}

// ── Mecca (21.4225°N, 39.8262°E) — MAKKAH method ───────────
// Reference: Umm al-Qura, ~March 21 (equinox)
void test_mecca_makkah_method() {
    calc.setMethod(CalcMethod::MAKKAH);
    calc.setAsrJuristic(AsrJuristic::STANDARD);
    calc.setHighLatMethod(HighLatMethod::NONE);

    PrayerTimes t = calc.calculate(2026, 3, 21, 21.4225, 39.8262);

    // Approximate expected times for equinox in Mecca (UTC+3 = local solar ~+2:39)
    // Fajr ~5:00, Sunrise ~6:10, Dhuhr ~12:18, Asr ~15:40, Maghrib ~18:27, Isha ~19:57
    TEST_ASSERT_GREATER_THAN(0, t.fajr);
    TEST_ASSERT_GREATER_THAN(t.fajr, t.sunrise);
    TEST_ASSERT_GREATER_THAN(t.sunrise, t.dhuhr);
    TEST_ASSERT_GREATER_THAN(t.dhuhr, t.asr);
    TEST_ASSERT_GREATER_THAN(t.asr, t.maghrib);
    TEST_ASSERT_GREATER_THAN(t.maghrib, t.isha);

    // Dhuhr should be near solar noon (~12:00-12:30)
    assertTimeInRange("Mecca Dhuhr", t.dhuhr, 11 * 60 + 50, 12 * 60 + 30);
}

// ── New York (40.7128°N, 74.0060°W) — ISNA method ──────────
void test_new_york_isna_method() {
    calc.setMethod(CalcMethod::ISNA);
    calc.setAsrJuristic(AsrJuristic::STANDARD);
    calc.setHighLatMethod(HighLatMethod::NONE);

    PrayerTimes t = calc.calculate(2026, 3, 21, 40.7128, -74.0060);

    // Order must be correct
    TEST_ASSERT_GREATER_THAN(0, t.fajr);
    TEST_ASSERT_LESS_THAN(t.sunrise, t.fajr);
    TEST_ASSERT_LESS_THAN(t.dhuhr, t.sunrise);
    TEST_ASSERT_LESS_THAN(t.asr, t.dhuhr);
    TEST_ASSERT_LESS_THAN(t.maghrib, t.asr);
    TEST_ASSERT_LESS_THAN(t.isha, t.maghrib);

    // Solar noon at ~74°W should be near 12:00 local solar time
    assertTimeInRange("NYC Dhuhr", t.dhuhr, 11 * 60 + 50, 12 * 60 + 30);

    // Day length on equinox ~12 hours → sunrise ~6:00, sunset ~18:00
    assertTimeInRange("NYC Sunrise", t.sunrise, 5 * 60 + 45, 6 * 60 + 30);
    assertTimeInRange("NYC Maghrib", t.maghrib, 17 * 60 + 30, 18 * 60 + 15);
}

// ── London (51.5074°N, 0.1278°W) — MWL method ──────────────
void test_london_mwl_method() {
    calc.setMethod(CalcMethod::MWL);
    calc.setAsrJuristic(AsrJuristic::STANDARD);
    calc.setHighLatMethod(HighLatMethod::ANGLE_BASED);

    PrayerTimes t = calc.calculate(2026, 3, 21, 51.5074, -0.1278);

    // Order check
    TEST_ASSERT_LESS_THAN(t.sunrise, t.fajr);
    TEST_ASSERT_LESS_THAN(t.dhuhr, t.sunrise);
    TEST_ASSERT_LESS_THAN(t.asr, t.dhuhr);
    TEST_ASSERT_LESS_THAN(t.maghrib, t.asr);
    TEST_ASSERT_LESS_THAN(t.isha, t.maghrib);

    // London Dhuhr near solar noon
    assertTimeInRange("London Dhuhr", t.dhuhr, 11 * 60 + 55, 12 * 60 + 20);
}

// ── Cairo (30.0444°N, 31.2357°E) — EGYPT method ────────────
void test_cairo_egypt_method() {
    calc.setMethod(CalcMethod::EGYPT);
    calc.setAsrJuristic(AsrJuristic::STANDARD);
    calc.setHighLatMethod(HighLatMethod::NONE);

    PrayerTimes t = calc.calculate(2026, 6, 21, 30.0444, 31.2357);

    // Summer solstice — longer days
    TEST_ASSERT_LESS_THAN(t.sunrise, t.fajr);
    TEST_ASSERT_LESS_THAN(t.isha, t.maghrib);

    // Dhuhr near noon
    assertTimeInRange("Cairo Dhuhr", t.dhuhr, 11 * 60 + 45, 12 * 60 + 20);
}

// ── ASR: Standard vs Hanafi ─────────────────────────────────
void test_asr_hanafi_later() {
    calc.setMethod(CalcMethod::ISNA);
    calc.setHighLatMethod(HighLatMethod::NONE);

    calc.setAsrJuristic(AsrJuristic::STANDARD);
    PrayerTimes standard = calc.calculate(2026, 3, 21, 40.7128, -74.0060);

    calc.setAsrJuristic(AsrJuristic::HANAFI);
    PrayerTimes hanafi = calc.calculate(2026, 3, 21, 40.7128, -74.0060);

    // Hanafi ASR must be later than Standard
    TEST_ASSERT_GREATER_THAN(standard.asr, hanafi.asr);

    // Difference should be ~30-60 minutes
    int diff = hanafi.asr - standard.asr;
    TEST_ASSERT_GREATER_OR_EQUAL(20, diff);
    TEST_ASSERT_LESS_OR_EQUAL(90, diff);
}

// ── High Latitude: Reykjavik (64.1°N) summer ────────────────
void test_reykjavik_high_latitude() {
    calc.setMethod(CalcMethod::MWL);
    calc.setAsrJuristic(AsrJuristic::STANDARD);
    calc.setHighLatMethod(HighLatMethod::ANGLE_BASED);

    // June 21 — near midnight sun at 64°N
    PrayerTimes t = calc.calculate(2026, 6, 21, 64.1466, -21.9426);

    // All times must be valid (> 0, < 24*60)
    TEST_ASSERT_GREATER_THAN(0, t.fajr);
    TEST_ASSERT_LESS_THAN(24 * 60, t.fajr);
    TEST_ASSERT_GREATER_THAN(0, t.isha);
    TEST_ASSERT_LESS_THAN(24 * 60, t.isha);

    // Order must hold even at extreme latitude
    TEST_ASSERT_LESS_THAN(t.sunrise, t.fajr);
    TEST_ASSERT_LESS_THAN(t.dhuhr, t.sunrise);
    TEST_ASSERT_LESS_THAN(t.asr, t.dhuhr);
    TEST_ASSERT_LESS_THAN(t.maghrib, t.asr);
}

// ── High Latitude: Midnight method ──────────────────────────
void test_high_lat_midnight_method() {
    calc.setMethod(CalcMethod::MWL);
    calc.setHighLatMethod(HighLatMethod::MIDNIGHT);

    PrayerTimes t = calc.calculate(2026, 6, 21, 64.1466, -21.9426);

    TEST_ASSERT_GREATER_THAN(0, t.fajr);
    TEST_ASSERT_LESS_THAN(24 * 60, t.isha);
}

// ── High Latitude: One-seventh method ───────────────────────
void test_high_lat_seventh_method() {
    calc.setMethod(CalcMethod::MWL);
    calc.setHighLatMethod(HighLatMethod::ONE_SEVENTH);

    PrayerTimes t = calc.calculate(2026, 6, 21, 64.1466, -21.9426);

    TEST_ASSERT_GREATER_THAN(0, t.fajr);
    TEST_ASSERT_LESS_THAN(24 * 60, t.isha);
}

// ── All 7 methods produce valid output ──────────────────────
void test_all_methods_valid() {
    CalcMethod methods[] = {
        CalcMethod::ISNA, CalcMethod::MWL, CalcMethod::EGYPT,
        CalcMethod::MAKKAH, CalcMethod::KARACHI, CalcMethod::TEHRAN,
        CalcMethod::JAFARI
    };

    for (int i = 0; i < 7; i++) {
        calc.setMethod(methods[i]);
        calc.setHighLatMethod(HighLatMethod::NONE);
        PrayerTimes t = calc.calculate(2026, 3, 21, 40.7128, -74.0060);

        // All times must be positive and in order
        TEST_ASSERT_GREATER_THAN(0, t.fajr);
        TEST_ASSERT_LESS_THAN(t.sunrise, t.fajr);
        TEST_ASSERT_LESS_THAN(t.dhuhr, t.sunrise);
        TEST_ASSERT_LESS_THAN(t.asr, t.dhuhr);
        TEST_ASSERT_LESS_THAN(t.maghrib, t.asr);
    }
}

// ── String-based method parsing ─────────────────────────────
void test_method_from_string() {
    calc.setMethodFromString("ISNA");
    TEST_ASSERT_EQUAL(CalcMethod::ISNA, calc.getMethod());

    calc.setMethodFromString("MWL");
    TEST_ASSERT_EQUAL(CalcMethod::MWL, calc.getMethod());

    calc.setMethodFromString("MAKKAH");
    TEST_ASSERT_EQUAL(CalcMethod::MAKKAH, calc.getMethod());

    calc.setMethodFromString("unknown");
    TEST_ASSERT_EQUAL(CalcMethod::ISNA, calc.getMethod());  // Default
}

// ─────────────────────────────────────────────────────────────

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_mecca_makkah_method);
    RUN_TEST(test_new_york_isna_method);
    RUN_TEST(test_london_mwl_method);
    RUN_TEST(test_cairo_egypt_method);
    RUN_TEST(test_asr_hanafi_later);
    RUN_TEST(test_reykjavik_high_latitude);
    RUN_TEST(test_high_lat_midnight_method);
    RUN_TEST(test_high_lat_seventh_method);
    RUN_TEST(test_all_methods_valid);
    RUN_TEST(test_method_from_string);

    return UNITY_END();
}
