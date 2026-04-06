#pragma once

#include <Arduino.h>

class ConfigManager;
class AudioManager;
class LedManager;

enum class IqamaState {
    IDLE,
    COUNTDOWN,
    PLAYING_IQAMA
};

class IqamaTimer {
public:
    void begin(ConfigManager* config, AudioManager* audio, LedManager* led);
    void update();

    // Start countdown for a specific prayer
    void startCountdown(int prayerIndex);
    void cancel();

    bool isActive() const { return _state != IqamaState::IDLE; }
    IqamaState getState() const { return _state; }
    int getRemainingSeconds() const;
    int getTotalSeconds() const { return _totalSeconds; }

    // For power-loss recovery
    void restoreState(int prayerIndex, unsigned long startTimestamp);

private:
    ConfigManager* _config = nullptr;
    AudioManager* _audio = nullptr;
    LedManager* _led = nullptr;

    IqamaState _state = IqamaState::IDLE;
    int _prayerIndex = -1;
    unsigned long _startTime = 0;
    int _totalSeconds = 0;
    int _iqamaTrack = 0;
};
