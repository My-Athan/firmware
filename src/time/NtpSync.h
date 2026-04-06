#pragma once

#include <Arduino.h>
#include <time.h>

class NtpSync {
public:
    void begin(const char* timezone);
    void update();

    bool isSynced() const { return _synced; }
    time_t now() const;
    struct tm localTime() const;

    int currentHour() const;
    int currentMinute() const;
    int currentDayOfWeek() const;   // 0=Sunday, 5=Friday, 6=Saturday
    int currentDay() const;
    int currentMonth() const;
    int currentYear() const;

    // Minutes since midnight (for prayer time comparison)
    int minutesSinceMidnight() const;

private:
    bool _synced = false;
    unsigned long _lastSyncAttempt = 0;
    const char* _timezone = "UTC";
};
