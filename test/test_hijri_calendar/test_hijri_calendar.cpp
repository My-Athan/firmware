#include <unity.h>
#include "../../src/prayer/HijriCalendar.h"

// ─────────────────────────────────────────────────────────────
// Hijri Calendar Unit Tests
// Verifies Gregorian→Hijri conversion against known dates.
// Tabular algorithm may differ ±1 day from observed calendars.
// ─────────────────────────────────────────────────────────────

#define HIJRI_TOLERANCE 1  // ±1 day

static void assertHijriDate(const char* label, HijriDate actual, int expYear, int expMonth, int expDay) {
    char msg[128];
    snprintf(msg, sizeof(msg), "%s: expected %d/%d/%d, got %d/%d/%d",
             label, expDay, expMonth, expYear, actual.day, actual.month, actual.year);

    TEST_ASSERT_EQUAL_INT_MESSAGE(expYear, actual.year, msg);
    TEST_ASSERT_EQUAL_INT_MESSAGE(expMonth, actual.month, msg);
    TEST_ASSERT_INT_WITHIN_MESSAGE(HIJRI_TOLERANCE, expDay, actual.day, msg);
}

// ── Known date conversions ──────────────────────────────────

// 2024-03-11 ≈ 1 Ramadan 1445
void test_ramadan_2024() {
    HijriDate h = HijriCalendar::gregorianToHijri(2024, 3, 11);
    TEST_ASSERT_EQUAL(1445, h.year);
    TEST_ASSERT_EQUAL(9, h.month);  // Ramadan
    TEST_ASSERT_INT_WITHIN(2, 1, h.day);
}

// 2024-04-10 ≈ 1 Shawwal 1445 (Eid al-Fitr)
void test_eid_fitr_2024() {
    HijriDate h = HijriCalendar::gregorianToHijri(2024, 4, 10);
    TEST_ASSERT_EQUAL(1445, h.year);
    TEST_ASSERT_EQUAL(10, h.month);  // Shawwal
    TEST_ASSERT_INT_WITHIN(2, 1, h.day);
}

// 2024-06-17 ≈ 10 Dhul Hijjah 1445 (Eid al-Adha)
void test_eid_adha_2024() {
    HijriDate h = HijriCalendar::gregorianToHijri(2024, 6, 17);
    TEST_ASSERT_EQUAL(1445, h.year);
    TEST_ASSERT_EQUAL(12, h.month);
    TEST_ASSERT_INT_WITHIN(2, 10, h.day);
}

// 2025-02-28 ≈ 1 Ramadan 1446
void test_ramadan_2025() {
    HijriDate h = HijriCalendar::gregorianToHijri(2025, 2, 28);
    TEST_ASSERT_EQUAL(1446, h.year);
    TEST_ASSERT_EQUAL(9, h.month);
    TEST_ASSERT_INT_WITHIN(2, 1, h.day);
}

// ── Ramadan detection ───────────────────────────────────────

void test_is_ramadan() {
    // During Ramadan 2024 (March 11 - April 9)
    HijriDate h1 = HijriCalendar::gregorianToHijri(2024, 3, 20);
    TEST_ASSERT_TRUE(HijriCalendar::isRamadan(h1));

    // Outside Ramadan
    HijriDate h2 = HijriCalendar::gregorianToHijri(2024, 5, 1);
    TEST_ASSERT_FALSE(HijriCalendar::isRamadan(h2));
}

// ── Holiday detection ───────────────────────────────────────

void test_eid_fitr_holiday() {
    HijriDate h = {1445, 10, 1};  // 1 Shawwal
    TEST_ASSERT_EQUAL(IslamicHoliday::EID_FITR, HijriCalendar::getHoliday(h));
}

void test_eid_adha_holiday() {
    HijriDate h = {1445, 12, 10};  // 10 Dhul Hijjah
    TEST_ASSERT_EQUAL(IslamicHoliday::EID_ADHA, HijriCalendar::getHoliday(h));
}

void test_mawlid_holiday() {
    HijriDate h = {1445, 3, 12};  // 12 Rabi al-Awwal
    TEST_ASSERT_EQUAL(IslamicHoliday::MAWLID, HijriCalendar::getHoliday(h));
}

void test_muharram_holiday() {
    HijriDate h = {1446, 1, 1};
    TEST_ASSERT_EQUAL(IslamicHoliday::MUHARRAM_1, HijriCalendar::getHoliday(h));
}

void test_ashura_holiday() {
    HijriDate h = {1446, 1, 10};
    TEST_ASSERT_EQUAL(IslamicHoliday::ASHURA, HijriCalendar::getHoliday(h));
}

void test_laylat_al_qadr_holiday() {
    HijriDate h = {1445, 9, 27};
    TEST_ASSERT_EQUAL(IslamicHoliday::LAYLAT_AL_QADR, HijriCalendar::getHoliday(h));
}

void test_isra_miraj_holiday() {
    HijriDate h = {1445, 7, 27};
    TEST_ASSERT_EQUAL(IslamicHoliday::ISRA_MIRAJ, HijriCalendar::getHoliday(h));
}

void test_no_holiday() {
    HijriDate h = {1445, 5, 15};  // Random date
    TEST_ASSERT_EQUAL(IslamicHoliday::NONE, HijriCalendar::getHoliday(h));
}

// ── Adjustment ──────────────────────────────────────────────

void test_hijri_adjustment() {
    HijriDate h1 = HijriCalendar::gregorianToHijri(2024, 3, 11, 0);
    HijriDate h2 = HijriCalendar::gregorianToHijri(2024, 3, 11, 1);
    HijriDate h3 = HijriCalendar::gregorianToHijri(2024, 3, 11, -1);

    // +1 adjustment should advance by 1 day
    TEST_ASSERT_EQUAL(h1.day + 1, h2.day);
    // -1 adjustment should go back by 1 day
    TEST_ASSERT_EQUAL(h1.day - 1, h3.day);
}

// ── Leap year ───────────────────────────────────────────────

void test_leap_years() {
    // Years 2, 5, 7, 10, 13, 16, 18, 21, 24, 26, 29 in 30-year cycle
    TEST_ASSERT_TRUE(HijriCalendar::isLeapYear(2));
    TEST_ASSERT_TRUE(HijriCalendar::isLeapYear(5));
    TEST_ASSERT_TRUE(HijriCalendar::isLeapYear(7));
    TEST_ASSERT_FALSE(HijriCalendar::isLeapYear(1));
    TEST_ASSERT_FALSE(HijriCalendar::isLeapYear(3));

    // Cycle-based: year 32 is in same position as year 2
    TEST_ASSERT_TRUE(HijriCalendar::isLeapYear(32));
}

// ── Month days ──────────────────────────────────────────────

void test_days_in_month() {
    // Odd months = 30 days, even months = 29 days
    TEST_ASSERT_EQUAL(30, HijriCalendar::daysInMonth(1445, 1));  // Muharram
    TEST_ASSERT_EQUAL(29, HijriCalendar::daysInMonth(1445, 2));  // Safar
    TEST_ASSERT_EQUAL(30, HijriCalendar::daysInMonth(1445, 9));  // Ramadan
    TEST_ASSERT_EQUAL(29, HijriCalendar::daysInMonth(1445, 12)); // Dhul Hijjah (non-leap)
}

// ── Month names ─────────────────────────────────────────────

void test_month_names() {
    TEST_ASSERT_EQUAL_STRING("Muharram", HijriCalendar::getMonthName(1));
    TEST_ASSERT_EQUAL_STRING("Ramadan", HijriCalendar::getMonthName(9));
    TEST_ASSERT_EQUAL_STRING("Dhul Hijjah", HijriCalendar::getMonthName(12));
}

// ── Holiday config keys ─────────────────────────────────────

void test_holiday_config_keys() {
    TEST_ASSERT_EQUAL_STRING("eidFitr", HijriCalendar::getHolidayConfigKey(IslamicHoliday::EID_FITR));
    TEST_ASSERT_EQUAL_STRING("eidAdha", HijriCalendar::getHolidayConfigKey(IslamicHoliday::EID_ADHA));
    TEST_ASSERT_EQUAL_STRING("mawlid", HijriCalendar::getHolidayConfigKey(IslamicHoliday::MAWLID));
    TEST_ASSERT_EQUAL_STRING("laylatAlQadr", HijriCalendar::getHolidayConfigKey(IslamicHoliday::LAYLAT_AL_QADR));
}

// ─────────────────────────────────────────────────────────────

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_ramadan_2024);
    RUN_TEST(test_eid_fitr_2024);
    RUN_TEST(test_eid_adha_2024);
    RUN_TEST(test_ramadan_2025);
    RUN_TEST(test_is_ramadan);
    RUN_TEST(test_eid_fitr_holiday);
    RUN_TEST(test_eid_adha_holiday);
    RUN_TEST(test_mawlid_holiday);
    RUN_TEST(test_muharram_holiday);
    RUN_TEST(test_ashura_holiday);
    RUN_TEST(test_laylat_al_qadr_holiday);
    RUN_TEST(test_isra_miraj_holiday);
    RUN_TEST(test_no_holiday);
    RUN_TEST(test_hijri_adjustment);
    RUN_TEST(test_leap_years);
    RUN_TEST(test_days_in_month);
    RUN_TEST(test_month_names);
    RUN_TEST(test_holiday_config_keys);

    return UNITY_END();
}
