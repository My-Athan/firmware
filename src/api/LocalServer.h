#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class ConfigManager;
class AudioManager;
class PrayerScheduler;
class NtpSync;

class LocalServer {
public:
    void begin(ConfigManager* config, AudioManager* audio,
               PrayerScheduler* scheduler, NtpSync* ntp);

    AsyncWebServer* getServer() { return &_server; }

private:
    AsyncWebServer _server{80};
    ConfigManager* _config = nullptr;
    AudioManager* _audio = nullptr;
    PrayerScheduler* _scheduler = nullptr;
    NtpSync* _ntp = nullptr;

    void _setupRoutes();

    // Route handlers
    void _handleStatus(AsyncWebServerRequest* req);
    void _handleTimetable(AsyncWebServerRequest* req);
    void _handleTrigger(AsyncWebServerRequest* req);
    void _handlePreview(AsyncWebServerRequest* req);
    void _handleVolume(AsyncWebServerRequest* req);
    void _handleGetConfig(AsyncWebServerRequest* req);
};
