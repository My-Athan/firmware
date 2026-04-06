#include "HolidayHandler.h"

void HolidayHandler::begin(ConfigManager* config) {
    _config = config;
}

void HolidayHandler::checkDate(int gYear, int gMonth, int gDay) {
    // Only recalculate once per day
    if (_lastCheckedDay == gDay) return;
    _lastCheckedDay = gDay;

    int adjustment = _config ? _config->getHijriAdjustment() : 0;
    _hijriDate = HijriCalendar::gregorianToHijri(gYear, gMonth, gDay, adjustment);
    _isRamadan = HijriCalendar::isRamadan(_hijriDate);
    _todayHoliday = HijriCalendar::getHoliday(_hijriDate);

    Serial.printf("[Hijri] %d/%d/%d %s",
        _hijriDate.day, _hijriDate.month, _hijriDate.year,
        HijriCalendar::getMonthName(_hijriDate.month));

    if (_isRamadan) Serial.print(" [RAMADAN]");
    if (_todayHoliday != IslamicHoliday::NONE) {
        Serial.printf(" [%s]", HijriCalendar::getHolidayName(_todayHoliday));
    }
    Serial.println();
}

const char* HolidayHandler::getTodayHolidayName() const {
    return HijriCalendar::getHolidayName(_todayHoliday);
}

int HolidayHandler::getHolidayTrack() const {
    if (_todayHoliday == IslamicHoliday::NONE || !_config) return 0;
    const char* key = HijriCalendar::getHolidayConfigKey(_todayHoliday);
    if (!key || strlen(key) == 0) return 0;
    return _config->getHolidayTrack(key);
}

int HolidayHandler::getPostAthanTrack() const {
    if (_todayHoliday == IslamicHoliday::NONE || !_config) return 0;
    const char* key = HijriCalendar::getHolidayConfigKey(_todayHoliday);
    if (!key || strlen(key) == 0) return 0;
    return _config->getHolidayPostAthanTrack(key);
}

bool HolidayHandler::shouldPlayHolidayTrack() const {
    if (_todayHoliday == IslamicHoliday::NONE || !_config) return false;
    const char* key = HijriCalendar::getHolidayConfigKey(_todayHoliday);
    if (!key || strlen(key) == 0) return false;
    return _config->isHolidayEnabled(key) && getHolidayTrack() > 0;
}
