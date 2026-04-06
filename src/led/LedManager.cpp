#include "LedManager.h"
#include "../config/defaults.h"

void LedManager::begin() {
    _pin = PIN_LED;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _state = LedState::OFF;
    Serial.println("[LED] Initialized");
}

void LedManager::update() {
    if (_state == LedState::OFF) {
        digitalWrite(_pin, LOW);
        return;
    }

    if (_state == LedState::PLAYING) {
        digitalWrite(_pin, HIGH);
        return;
    }

    unsigned long interval = _getInterval();
    unsigned long now = millis();

    if (now - _lastToggle >= interval) {
        _lastToggle = now;
        _applyPattern();
    }
}

void LedManager::setState(LedState state) {
    if (_state == state) return;
    _state = state;
    _lastToggle = millis();
    _ledOn = false;

    const char* names[] = {"OFF","IDLE","PRE_ATHAN","PLAYING","IQAMA","ERROR","NO_WIFI","RECOVERY","PROVISIONING"};
    Serial.printf("[LED] State → %s\n", names[(int)state]);
}

void LedManager::setIqamaRemaining(int totalSec, int remainingSec) {
    _iqamaTotalSec = totalSec;
    _iqamaRemainSec = remainingSec;
}

void LedManager::_applyPattern() {
    switch (_state) {
        case LedState::IDLE:
            // Brief flash every 5 seconds
            _ledOn = !_ledOn;
            digitalWrite(_pin, _ledOn ? HIGH : LOW);
            break;

        case LedState::PRE_ATHAN:
        case LedState::NO_WIFI:
            // Slow blink / pulse
            _ledOn = !_ledOn;
            digitalWrite(_pin, _ledOn ? HIGH : LOW);
            break;

        case LedState::IQAMA_COUNTDOWN:
            // Pulse that accelerates
            _ledOn = !_ledOn;
            digitalWrite(_pin, _ledOn ? HIGH : LOW);
            break;

        case LedState::ERROR:
            // Fast blink
            _ledOn = !_ledOn;
            digitalWrite(_pin, _ledOn ? HIGH : LOW);
            break;

        case LedState::RECOVERY:
            // Double blink pattern: on-off-on-off-pause
            static uint8_t recoveryStep = 0;
            recoveryStep = (recoveryStep + 1) % 6;
            digitalWrite(_pin, (recoveryStep < 4 && recoveryStep % 2 == 0) ? HIGH : LOW);
            break;

        case LedState::PROVISIONING:
            // Triple blink
            static uint8_t provStep = 0;
            provStep = (provStep + 1) % 8;
            digitalWrite(_pin, (provStep < 6 && provStep % 2 == 0) ? HIGH : LOW);
            break;

        default:
            break;
    }
}

unsigned long LedManager::_getInterval() const {
    switch (_state) {
        case LedState::IDLE:          return 5000;   // Flash every 5s
        case LedState::PRE_ATHAN:     return 1000;   // 1s slow blink
        case LedState::NO_WIFI:       return 1500;   // 1.5s pulse
        case LedState::ERROR:         return 200;    // Fast blink
        case LedState::RECOVERY:      return 200;    // Double blink timing
        case LedState::PROVISIONING:  return 150;    // Triple blink timing

        case LedState::IQAMA_COUNTDOWN: {
            // Accelerate from 2000ms down to 100ms as countdown approaches zero
            if (_iqamaTotalSec <= 0) return 500;
            float ratio = (float)_iqamaRemainSec / (float)_iqamaTotalSec;
            unsigned long interval = 100 + (unsigned long)(ratio * 1900.0f);
            return interval;
        }

        default: return 1000;
    }
}
