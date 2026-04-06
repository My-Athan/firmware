#pragma once

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
// Hijri Calendar — Tabular (Kuwaiti) algorithm
// Deterministic, no astronomical observation needed.
// Pure integer arithmetic, ~80 lines.
// ─────────────────────────────────────────────────────────────

struct HijriDate {
    int year;
    int month;   // 1-12
    int day;     // 1-30
};

// Islamic holidays
enum class IslamicHoliday {
    NONE,
    MUHARRAM_1,       // 1 Muharram — Islamic New Year
    ASHURA,           // 10 Muharram
    MAWLID,           // 12 Rabi al-Awwal
    ISRA_MIRAJ,       // 27 Rajab
    RAMADAN_START,    // 1 Ramadan
    LAYLAT_AL_QADR,   // 27 Ramadan
    EID_FITR,         // 1 Shawwal
    EID_ADHA          // 10 Dhul Hijjah
};

// Hijri month names (for display)
static const char* HIJRI_MONTH_NAMES[] = {
    "", "Muharram", "Safar", "Rabi al-Awwal", "Rabi al-Thani",
    "Jumada al-Ula", "Jumada al-Thani", "Rajab", "Sha'ban",
    "Ramadan", "Shawwal", "Dhul Qi'dah", "Dhul Hijjah"
};

class HijriCalendar {
public:
    // Convert Gregorian to Hijri with optional adjustment (-2 to +2 days)
    static HijriDate gregorianToHijri(int gYear, int gMonth, int gDay, int adjustment = 0);

    // Check special dates
    static bool isRamadan(const HijriDate& date);
    static IslamicHoliday getHoliday(const HijriDate& date);
    static const char* getHolidayName(IslamicHoliday holiday);
    static const char* getHolidayConfigKey(IslamicHoliday holiday);

    // Month info
    static const char* getMonthName(int month);
    static int daysInMonth(int hYear, int hMonth);
    static bool isLeapYear(int hYear);

private:
    static int _gregorianToJulianDay(int year, int month, int day);
    static HijriDate _julianDayToHijri(int jd);
};
