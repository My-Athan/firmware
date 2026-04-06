#include "HijriCalendar.h"

// ─────────────────────────────────────────────────────────────
// Tabular Islamic Calendar (30-year cycle)
// Leap years in cycle: 2, 5, 7, 10, 13, 16, 18, 21, 24, 26, 29
// Average month = 29.53056 days (10,631 days / 360 months)
// ─────────────────────────────────────────────────────────────

static const int LEAP_YEARS[] = {2, 5, 7, 10, 13, 16, 18, 21, 24, 26, 29};
static const int NUM_LEAP = 11;

bool HijriCalendar::isLeapYear(int hYear) {
    int cycle = ((hYear - 1) % 30) + 1;
    for (int i = 0; i < NUM_LEAP; i++) {
        if (cycle == LEAP_YEARS[i]) return true;
    }
    return false;
}

int HijriCalendar::daysInMonth(int hYear, int hMonth) {
    if (hMonth < 1 || hMonth > 12) return 30;
    // Odd months = 30 days, even months = 29 days
    // Exception: month 12 in leap years = 30 days
    if (hMonth == 12 && isLeapYear(hYear)) return 30;
    return (hMonth % 2 == 1) ? 30 : 29;
}

int HijriCalendar::_gregorianToJulianDay(int year, int month, int day) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    int A = year / 100;
    int B = 2 - A + A / 4;
    return (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + B - 1524;
}

HijriDate HijriCalendar::_julianDayToHijri(int jd) {
    // Hijri epoch: July 16, 622 CE (Julian) = JD 1948439.5
    // Using integer approximation: JD 1948440
    int l = jd - 1948440 + 10632;
    int n = (l - 1) / 10631;
    l = l - 10631 * n + 354;

    int j = ((10985 - l) / 5316) * ((50 * l) / 17719)
          + ((l / 5670) * ((43 * l) / 15238));
    l = l - ((30 - j) / 15) * ((17719 * j) / 50)
      - ((j / 16) * ((15238 * j) / 43)) + 29;

    int month = (24 * l) / 709;
    int day = l - (709 * month) / 24;
    int year = 30 * n + j - 30;

    HijriDate date;
    date.year = year;
    date.month = month;
    date.day = day;
    return date;
}

HijriDate HijriCalendar::gregorianToHijri(int gYear, int gMonth, int gDay, int adjustment) {
    int jd = _gregorianToJulianDay(gYear, gMonth, gDay);
    jd += adjustment;  // Apply user's Hijri date correction
    return _julianDayToHijri(jd);
}

bool HijriCalendar::isRamadan(const HijriDate& date) {
    return date.month == 9;
}

IslamicHoliday HijriCalendar::getHoliday(const HijriDate& date) {
    if (date.month == 1 && date.day == 1)    return IslamicHoliday::MUHARRAM_1;
    if (date.month == 1 && date.day == 10)   return IslamicHoliday::ASHURA;
    if (date.month == 3 && date.day == 12)   return IslamicHoliday::MAWLID;
    if (date.month == 7 && date.day == 27)   return IslamicHoliday::ISRA_MIRAJ;
    if (date.month == 9 && date.day == 1)    return IslamicHoliday::RAMADAN_START;
    if (date.month == 9 && date.day == 27)   return IslamicHoliday::LAYLAT_AL_QADR;
    if (date.month == 10 && date.day == 1)   return IslamicHoliday::EID_FITR;
    if (date.month == 12 && date.day == 10)  return IslamicHoliday::EID_ADHA;
    return IslamicHoliday::NONE;
}

const char* HijriCalendar::getHolidayName(IslamicHoliday h) {
    switch (h) {
        case IslamicHoliday::MUHARRAM_1:     return "Islamic New Year";
        case IslamicHoliday::ASHURA:         return "Ashura";
        case IslamicHoliday::MAWLID:         return "Mawlid al-Nabi";
        case IslamicHoliday::ISRA_MIRAJ:     return "Isra & Mi'raj";
        case IslamicHoliday::RAMADAN_START:  return "Ramadan Begins";
        case IslamicHoliday::LAYLAT_AL_QADR: return "Laylat al-Qadr";
        case IslamicHoliday::EID_FITR:       return "Eid al-Fitr";
        case IslamicHoliday::EID_ADHA:       return "Eid al-Adha";
        default:                              return "";
    }
}

const char* HijriCalendar::getHolidayConfigKey(IslamicHoliday h) {
    switch (h) {
        case IslamicHoliday::MUHARRAM_1:     return "muharram";
        case IslamicHoliday::ASHURA:         return "ashura";
        case IslamicHoliday::MAWLID:         return "mawlid";
        case IslamicHoliday::ISRA_MIRAJ:     return "israMiraj";
        case IslamicHoliday::LAYLAT_AL_QADR: return "laylatAlQadr";
        case IslamicHoliday::EID_FITR:       return "eidFitr";
        case IslamicHoliday::EID_ADHA:       return "eidAdha";
        default:                              return "";
    }
}

const char* HijriCalendar::getMonthName(int month) {
    if (month < 1 || month > 12) return "";
    return HIJRI_MONTH_NAMES[month];
}
