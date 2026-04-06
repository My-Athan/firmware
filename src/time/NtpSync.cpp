#include "NtpSync.h"
#include "../config/defaults.h"

void NtpSync::begin(const char* timezone) {
    _timezone = timezone;
    configTzTime(_timezone, NTP_SERVER_1, NTP_SERVER_2);
    Serial.printf("[NTP] Configured timezone: %s\n", _timezone);
    _lastSyncAttempt = millis();
}

void NtpSync::update() {
    if (_synced && (millis() - _lastSyncAttempt < NTP_SYNC_INTERVAL_MS)) {
        return;
    }

    struct tm timeinfo;
    // Use short timeout in loop to avoid blocking (100ms vs 5000ms)
    int timeout = _synced ? 100 : 5000;
    if (getLocalTime(&timeinfo, timeout)) {
        if (!_synced) {
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("[NTP] Synced: %s\n", buf);
        }
        _synced = true;
    } else {
        Serial.println("[NTP] Sync failed, will retry");
        _synced = false;
    }
    _lastSyncAttempt = millis();
}

time_t NtpSync::now() const {
    time_t t;
    time(&t);
    return t;
}

struct tm NtpSync::localTime() const {
    struct tm timeinfo;
    getLocalTime(&timeinfo, 0);
    return timeinfo;
}

int NtpSync::currentHour() const {
    struct tm t = localTime();
    return t.tm_hour;
}

int NtpSync::currentMinute() const {
    struct tm t = localTime();
    return t.tm_min;
}

int NtpSync::currentDayOfWeek() const {
    struct tm t = localTime();
    return t.tm_wday;  // 0=Sunday ... 5=Friday
}

int NtpSync::currentDay() const {
    struct tm t = localTime();
    return t.tm_mday;
}

int NtpSync::currentMonth() const {
    struct tm t = localTime();
    return t.tm_mon + 1;
}

int NtpSync::currentYear() const {
    struct tm t = localTime();
    return t.tm_year + 1900;
}

int NtpSync::minutesSinceMidnight() const {
    struct tm t = localTime();
    return t.tm_hour * 60 + t.tm_min;
}
