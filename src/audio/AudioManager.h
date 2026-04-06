#pragma once

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

enum class AudioState {
    IDLE,
    PLAYING,
    PREVIEW,     // Timed playback (10s)
    WAITING,     // Between athan and doaa
    ERROR
};

class AudioManager {
public:
    bool begin();
    void update();

    void play(int track);
    void playPreview(int track);        // Play for PREVIEW_DURATION_MS then stop
    void stop();
    void setVolume(int vol);            // 0-30
    void setVolumeForPrayer(int prayerIndex, int globalVolume, int perPrayerVolume);

    bool isPlaying() const { return _state == AudioState::PLAYING || _state == AudioState::PREVIEW; }
    bool isIdle() const { return _state == AudioState::IDLE; }
    AudioState getState() const { return _state; }
    bool hasError() const { return _state == AudioState::ERROR; }

    int getCurrentTrack() const { return _currentTrack; }

private:
    DFRobotDFPlayerMini _player;
    HardwareSerial* _serial = nullptr;
    AudioState _state = AudioState::IDLE;
    int _currentTrack = 0;
    int _currentVolume = 20;
    unsigned long _playStartTime = 0;
    unsigned long _previewDuration = 0;
    bool _initialized = false;

    void _checkPlaybackFinished();
};
