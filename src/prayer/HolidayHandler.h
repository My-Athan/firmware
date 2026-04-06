#pragma once

#include <Arduino.h>
#include "HijriCalendar.h"
#include "../config/ConfigManager.h"
#include "../audio/AudioManager.h"

class HolidayHandler {
public:
    void begin(ConfigManager* config);
    void checkDate(int gYear, int gMonth, int gDay);

    bool isHolidayToday() const { return _todayHoliday != IslamicHoliday::NONE; }
    IslamicHoliday getTodayHoliday() const { return _todayHoliday; }
    const char* getTodayHolidayName() const;

    bool isRamadan() const { return _isRamadan; }
    const HijriDate& getHijriDate() const { return _hijriDate; }

    // Get the track to play for current holiday (0 = no special track)
    int getHolidayTrack() const;

    // Get post-athan track (e.g., takbeer after Eid athan)
    int getPostAthanTrack() const;

    // Should we play the holiday track?
    bool shouldPlayHolidayTrack() const;

private:
    ConfigManager* _config = nullptr;
    HijriDate _hijriDate = {0, 0, 0};
    IslamicHoliday _todayHoliday = IslamicHoliday::NONE;
    bool _isRamadan = false;
    int _lastCheckedDay = -1;
};
