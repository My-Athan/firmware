#include "ButtonHandler.h"
#include "../config/defaults.h"

void ButtonHandler::begin() {
    _pin = PIN_BUTTON;
    pinMode(_pin, INPUT_PULLUP);
    Serial.println("[Button] Initialized on GPIO" + String(_pin));
}

ButtonEvent ButtonHandler::update() {
    bool currentState = digitalRead(_pin);
    unsigned long now = millis();

    // Debounce
    if (currentState != _lastState) {
        delay(BUTTON_DEBOUNCE_MS);
        currentState = digitalRead(_pin);
    }

    // Button pressed (LOW = pressed with pull-up)
    if (currentState == LOW && _lastState == HIGH) {
        _buttonDown = true;
        _pressStart = now;
        _longPressFired = false;
    }

    // Button held — check for long press
    if (_buttonDown && currentState == LOW && !_longPressFired) {
        if (now - _pressStart >= BUTTON_LONG_PRESS_MS) {
            _longPressFired = true;
            _pressCount = 0;
            _lastState = currentState;
            Serial.println("[Button] Long press → WiFi reset");
            return ButtonEvent::LONG_PRESS;
        }
    }

    // Button released
    if (currentState == HIGH && _lastState == LOW) {
        _buttonDown = false;

        if (!_longPressFired) {
            _pressCount++;
            _lastRelease = now;
        }
    }

    // Evaluate press count after double-press window expires
    if (_pressCount > 0 && !_buttonDown && (now - _lastRelease > BUTTON_DOUBLE_PRESS_MS)) {
        ButtonEvent evt = ButtonEvent::NONE;
        if (_pressCount >= 2) {
            evt = ButtonEvent::DOUBLE_PRESS;
            Serial.println("[Button] Double press → Preview");
        } else {
            evt = ButtonEvent::SHORT_PRESS;
            Serial.println("[Button] Short press → Trigger");
        }
        _pressCount = 0;
        _lastState = currentState;
        return evt;
    }

    _lastState = currentState;
    return ButtonEvent::NONE;
}
