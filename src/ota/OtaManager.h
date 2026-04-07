#pragma once

#include <Arduino.h>

class ConfigManager;
class NtpSync;
class LedManager;

enum class OtaState {
    IDLE,
    CHECKING,
    DOWNLOADING,
    VERIFYING,
    FLASHING,
    REBOOTING,
    ERROR
};

struct OtaUpdateInfo {
    char version[20];
    char sha256[65];
    char url[256];
    int size;
};

class OtaManager {
public:
    void begin(ConfigManager* config, NtpSync* ntp, LedManager* led);

    // Check and apply update if available
    bool checkAndUpdate(const char* currentVersion);

    // Apply a specific update (called by BackendClient)
    bool applyUpdate(const OtaUpdateInfo& info);

    OtaState getState() const { return _state; }
    int getProgress() const { return _progress; }
    const char* getError() const { return _error; }
    bool justUpdated() const { return _justUpdated; }

private:
    ConfigManager* _config = nullptr;
    NtpSync* _ntp = nullptr;
    LedManager* _led = nullptr;

    OtaState _state = OtaState::IDLE;
    int _progress = 0;  // 0-100
    char _error[128] = "";
    bool _justUpdated = false;

    bool _download(const char* url, int expectedSize);
    bool _verifySha256(const char* expected);
    void _markBootSuccessful();

    static int _consecutiveFailures;
};
