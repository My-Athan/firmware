#pragma once

#include <Arduino.h>
#include "PrayerCalculator.h"
#include "HijriCalendar.h"
#include "HolidayHandler.h"
#include "IqamaTimer.h"

class ConfigManager;
class AudioManager;
class LedManager;
class NtpSync;

enum class SchedulerState {
    IDLE,
    PRE_ATHAN,       // LED pre-warning active
    PLAYING_ATHAN,   // Athan audio playing
    POST_ATHAN,      // Holiday post-athan track or doaa
    IQAMA_WAIT       // Iqama timer running (handled by IqamaTimer)
};

class PrayerScheduler {
public:
    void begin(ConfigManager* config, AudioManager* audio, LedManager* led,
               NtpSync* ntp, IqamaTimer* iqama);
    void tick();    // Call from loop() every iteration

    // Manual trigger
    void triggerAthan(int prayerIndex);
    void triggerPreview(int track);

    // State queries
    SchedulerState getState() const { return _state; }
    int getNextPrayerIndex() const;
    int getNextPrayerMinutes() const;
    const PrayerTimes& getTodayTimes() const { return _todayTimes; }
    const PrayerTimes& getTomorrowTimes() const { return _tomorrowTimes; }
    const HolidayHandler& getHolidayHandler() const { return _holiday; }

    // Format prayer time as HH:MM string
    static void formatTime(int minutesSinceMidnight, char* buf, size_t len);

private:
    ConfigManager* _config = nullptr;
    AudioManager* _audio = nullptr;
    LedManager* _led = nullptr;
    NtpSync* _ntp = nullptr;
    IqamaTimer* _iqama = nullptr;

    PrayerCalculator _calculator;
    HolidayHandler _holiday;
    SchedulerState _state = SchedulerState::IDLE;

    PrayerTimes _todayTimes = {};
    PrayerTimes _tomorrowTimes = {};
    int _playedTodayMask = 0;
    int _lastCalcDay = -1;
    unsigned long _lastTickMs = 0;

    // Current playback context
    int _currentPrayerIndex = -1;
    int _postAthanTrack = 0;

    void _recalculate();
    void _checkPrayerTimes(int nowMinutes, int dayOfWeek);
    void _checkSuhoorAlert(int nowMinutes);
    void _checkPreAthan(int nowMinutes);
    void _playAthan(int prayerIndex, int dayOfWeek);
    void _onAthanFinished();
    int _getPrayerMinutes(const PrayerTimes& times, int index) const;
    void _persistRecovery();
    void _restoreRecovery();
};
