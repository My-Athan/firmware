#include "AudioManager.h"
#include "../config/defaults.h"

bool AudioManager::begin() {
    _serial = new HardwareSerial(DFPLAYER_UART);
    _serial->begin(DFPLAYER_BAUD, SERIAL_8N1, PIN_DFPLAYER_RX, PIN_DFPLAYER_TX);

    delay(500);  // DFPlayer needs time to initialize after power-on

    if (!_player.begin(*_serial, true, true)) {
        Serial.println("[Audio] DFPlayer init FAILED — check wiring and SD card");
        _state = AudioState::ERROR;
        return false;
    }

    _player.setTimeOut(500);
    _player.volume(_currentVolume);
    _player.EQ(DFPLAYER_EQ_NORMAL);
    _player.outputDevice(DFPLAYER_DEVICE_SD);

    _initialized = true;
    _state = AudioState::IDLE;

    int fileCount = _player.readFileCounts();
    Serial.printf("[Audio] DFPlayer ready. SD card files: %d\n", fileCount);
    return true;
}

void AudioManager::update() {
    if (!_initialized) return;

    // Check preview timeout
    if (_state == AudioState::PREVIEW) {
        if (millis() - _playStartTime >= _previewDuration) {
            Serial.println("[Audio] Preview finished");
            stop();
            return;
        }
    }

    _checkPlaybackFinished();
}

void AudioManager::play(int track) {
    if (!_initialized) return;
    if (track <= 0) {
        Serial.println("[Audio] Invalid track number");
        return;
    }

    _currentTrack = track;
    _player.play(track);
    _playStartTime = millis();
    _state = AudioState::PLAYING;
    Serial.printf("[Audio] Playing track %d at volume %d\n", track, _currentVolume);
}

void AudioManager::playPreview(int track) {
    if (!_initialized) return;
    if (track <= 0) return;

    _currentTrack = track;
    _player.play(track);
    _playStartTime = millis();
    _previewDuration = PREVIEW_DURATION_MS;
    _state = AudioState::PREVIEW;
    Serial.printf("[Audio] Preview track %d for %lums\n", track, _previewDuration);
}

void AudioManager::stop() {
    if (!_initialized) return;
    _player.stop();
    _state = AudioState::IDLE;
    _currentTrack = 0;
}

void AudioManager::setVolume(int vol) {
    vol = constrain(vol, 0, 30);
    _currentVolume = vol;
    if (_initialized) {
        _player.volume(vol);
    }
}

void AudioManager::setVolumeForPrayer(int prayerIndex, int globalVolume, int perPrayerVolume) {
    // Per-prayer volume overrides global if set (non-zero)
    int vol = (perPrayerVolume > 0) ? perPrayerVolume : globalVolume;
    setVolume(vol);
}

void AudioManager::_checkPlaybackFinished() {
    if (_state != AudioState::PLAYING) return;

    // DFPlayer sends status via UART when track finishes
    if (_player.available()) {
        uint8_t type = _player.readType();
        if (type == DFPlayerPlayFinished) {
            Serial.printf("[Audio] Track %d finished\n", _currentTrack);
            _state = AudioState::IDLE;
            _currentTrack = 0;
        } else if (type == DFPlayerError) {
            Serial.printf("[Audio] Error: %d\n", _player.read());
            _state = AudioState::ERROR;
        }
    }
}
