#pragma once

#include <Arduino.h>

enum class ButtonEvent {
    NONE,
    SHORT_PRESS,     // Manual athan trigger
    DOUBLE_PRESS,    // Athan preview
    LONG_PRESS       // WiFi reset
};

class ButtonHandler {
public:
    void begin();
    ButtonEvent update();   // Call from loop(), returns event if any

private:
    uint8_t _pin = PIN_BUTTON;
    bool _lastState = HIGH;       // Pull-up: HIGH = not pressed
    bool _buttonDown = false;
    unsigned long _pressStart = 0;
    unsigned long _lastRelease = 0;
    int _pressCount = 0;
    bool _longPressFired = false;
};
