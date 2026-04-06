#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigManager;
class NtpSync;
class MultiRoomSync;

class BackendClient {
public:
    void begin(ConfigManager* config, NtpSync* ntp, MultiRoomSync* sync);
    void update();   // Call from loop() — handles polling intervals

    // Manual triggers
    bool registerDevice();
    bool fetchConfig();
    bool sendHeartbeat();
    bool fetchTimetable();
    bool checkOtaUpdate();

    bool isRegistered() const { return _registered; }
    bool hasApiKey() const { return _apiKey.length() > 0; }
    const char* getLastError() const { return _lastError; }

private:
    ConfigManager* _config = nullptr;
    NtpSync* _ntp = nullptr;
    MultiRoomSync* _sync = nullptr;

    String _apiKey;
    String _baseUrl;
    bool _registered = false;
    char _lastError[128] = "";

    unsigned long _lastConfigPoll = 0;
    unsigned long _lastHeartbeat = 0;
    unsigned long _lastOtaCheck = 0;

    // HTTP helpers
    bool _httpGet(const char* path, JsonDocument& response);
    bool _httpPost(const char* path, const JsonDocument& body, JsonDocument& response);
    bool _httpPut(const char* path, const JsonDocument& body, JsonDocument& response);
    void _setAuthHeaders();
};
