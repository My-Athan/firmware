#pragma once

#include <Arduino.h>

enum class LedState {
    OFF,
    IDLE,
    PRE_ATHAN,        // Slow blink — prayer approaching
    PLAYING,          // Solid on — athan playing
    IQAMA_COUNTDOWN,  // Pulse — accelerates as time approaches
    ERROR,            // Fast blink
    NO_WIFI,          // Slow pulse
    RECOVERY,         // Double blink
    PROVISIONING      // Triple blink — BLE/WiFi setup
};

class LedManager {
public:
    void begin();
    void update();

    void setState(LedState state);
    LedState getState() const { return _state; }

    // For iqama countdown: set remaining seconds to modulate pulse speed
    void setIqamaRemaining(int totalSec, int remainingSec);

private:
    LedState _state = LedState::OFF;
    uint8_t _pin;
    bool _ledOn = false;
    unsigned long _lastToggle = 0;
    unsigned long _interval = 1000;

    // Iqama pulse acceleration
    int _iqamaTotalSec = 0;
    int _iqamaRemainSec = 0;

    void _applyPattern();
    unsigned long _getInterval() const;
};
