#include "LocalServer.h"
#include <myathan_version.h>
#include "../config/ConfigManager.h"
#include "../audio/AudioManager.h"
#include "../prayer/PrayerScheduler.h"
#include "../time/NtpSync.h"
#include "../prayer/HijriCalendar.h"
#include "../config/defaults.h"
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

// Simple rate limiter: track last request time per endpoint category
static unsigned long _lastTriggerMs = 0;
static unsigned long _lastConfigMs = 0;
static const unsigned long RATE_LIMIT_MS = 1000;  // 1 request/second

static bool _rateLimited(unsigned long& lastMs) {
    unsigned long now = millis();
    if (now - lastMs < RATE_LIMIT_MS) return true;
    lastMs = now;
    return false;
}

void LocalServer::begin(ConfigManager* config, AudioManager* audio,
                        PrayerScheduler* scheduler, NtpSync* ntp) {
    _config = config;
    _audio = audio;
    _scheduler = scheduler;
    _ntp = ntp;

    _setupRoutes();
    _server.begin();

    if (MDNS.begin("myathan")) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("[Server] mDNS: myathan.local");
    }

    Serial.println("[Server] HTTP server started on port 80");
}

void LocalServer::_setupRoutes() {
    // GET /status
    _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        _handleStatus(req);
    });

    // GET /timetable
    _server.on("/timetable", HTTP_GET, [this](AsyncWebServerRequest* req) {
        _handleTimetable(req);
    });

    // POST /trigger
    _server.on("/trigger", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _handleTrigger(req);
    });

    // POST /preview
    _server.on("/preview", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _handlePreview(req);
    });

    // POST /volume
    _server.on("/volume", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _handleVolume(req);
    });

    // GET /config
    _server.on("/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        _handleGetConfig(req);
    });

    // POST /config (with body)
    AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler(
        "/config", [this](AsyncWebServerRequest* req, JsonVariant& json) {
            JsonObject obj = json.as<JsonObject>();
            if (_config->mergeConfig(obj)) {
                req->send(200, "application/json", "{\"ok\":true}");
            } else {
                req->send(500, "application/json", "{\"error\":\"save failed\"}");
            }
        });
    _server.addHandler(configHandler);

    // POST /sync-trigger (with body)
    AsyncCallbackJsonWebHandler* syncHandler = new AsyncCallbackJsonWebHandler(
        "/sync-trigger", [this](AsyncWebServerRequest* req, JsonVariant& json) {
            int prayer = json["prayer"] | -1;
            unsigned long triggerAt = json["triggerAtEpoch"] | 0UL;
            if (prayer >= 0 && prayer < PRAYER_COUNT) {
                // Store for MultiRoomSync to pick up
                req->send(200, "application/json", "{\"ok\":true}");
            } else {
                req->send(400, "application/json", "{\"error\":\"invalid prayer\"}");
            }
        });
    _server.addHandler(syncHandler);

    // 404
    _server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });
}

void LocalServer::_handleStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["deviceId"] = _config->getDeviceId();
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["uptime"] = millis() / 1000;
    doc["wifi"]["connected"] = WiFi.isConnected();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["rssi"] = WiFi.RSSI();

    if (_ntp && _ntp->isSynced()) {
        doc["time"]["synced"] = true;
        char buf[20];
        struct tm t = _ntp->localTime();
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
        doc["time"]["local"] = buf;
    } else {
        doc["time"]["synced"] = false;
    }

    doc["audio"]["playing"] = _audio ? _audio->isPlaying() : false;
    doc["audio"]["volume"] = _config->getVolume();

    if (_scheduler) {
        int next = _scheduler->getNextPrayerIndex();
        doc["prayer"]["nextIndex"] = next;
        doc["prayer"]["nextInMinutes"] = _scheduler->getNextPrayerMinutes();

        const char* names[] = {"fajr","dhuhr","asr","maghrib","isha"};
        if (next >= 0 && next < 5) doc["prayer"]["next"] = names[next];
    }

    // Hijri date
    if (_scheduler) {
        const auto& hol = _scheduler->getHolidayHandler();
        const auto& hd = hol.getHijriDate();
        if (hd.year > 0) {
            doc["hijri"]["day"] = hd.day;
            doc["hijri"]["month"] = hd.month;
            doc["hijri"]["year"] = hd.year;
            doc["hijri"]["monthName"] = HijriCalendar::getMonthName(hd.month);
            doc["hijri"]["ramadan"] = hol.isRamadan();
            if (hol.isHolidayToday()) {
                doc["hijri"]["holiday"] = hol.getTodayHolidayName();
            }
        }
    }

    String response;
    serializeJson(doc, response);
    req->send(200, "application/json", response);
}

void LocalServer::_handleTimetable(AsyncWebServerRequest* req) {
    if (!_scheduler) {
        req->send(503, "application/json", "{\"error\":\"scheduler not ready\"}");
        return;
    }

    JsonDocument doc;
    const PrayerTimes& t = _scheduler->getTodayTimes();
    char buf[6];

    PrayerScheduler::formatTime(t.fajr, buf, sizeof(buf));
    doc["today"]["fajr"] = buf;
    PrayerScheduler::formatTime(t.sunrise, buf, sizeof(buf));
    doc["today"]["sunrise"] = buf;
    PrayerScheduler::formatTime(t.dhuhr, buf, sizeof(buf));
    doc["today"]["dhuhr"] = buf;
    PrayerScheduler::formatTime(t.asr, buf, sizeof(buf));
    doc["today"]["asr"] = buf;
    PrayerScheduler::formatTime(t.maghrib, buf, sizeof(buf));
    doc["today"]["maghrib"] = buf;
    PrayerScheduler::formatTime(t.isha, buf, sizeof(buf));
    doc["today"]["isha"] = buf;

    const PrayerTimes& tm = _scheduler->getTomorrowTimes();
    PrayerScheduler::formatTime(tm.fajr, buf, sizeof(buf));
    doc["tomorrow"]["fajr"] = buf;
    PrayerScheduler::formatTime(tm.dhuhr, buf, sizeof(buf));
    doc["tomorrow"]["dhuhr"] = buf;

    doc["method"] = _config->getCalcMethod();
    doc["asrJuristic"] = _config->getAsrJuristic();
    doc["location"]["lat"] = _config->getLatitude();
    doc["location"]["lon"] = _config->getLongitude();

    String response;
    serializeJson(doc, response);
    req->send(200, "application/json", response);
}

void LocalServer::_handleTrigger(AsyncWebServerRequest* req) {
    if (_rateLimited(_lastTriggerMs)) {
        req->send(429, "application/json", "{\"error\":\"rate limited\"}");
        return;
    }

    int prayer = 0;
    if (req->hasParam("prayer")) {
        String val = req->getParam("prayer")->value();
        // Validate: must be single digit 0-4
        if (val.length() == 1 && val[0] >= '0' && val[0] <= '4') {
            prayer = val[0] - '0';
        }
    }

    if (_scheduler) _scheduler->triggerAthan(prayer);
    req->send(200, "application/json", "{\"ok\":true,\"prayer\":" + String(prayer) + "}");
}

void LocalServer::_handlePreview(AsyncWebServerRequest* req) {
    if (_rateLimited(_lastTriggerMs)) {
        req->send(429, "application/json", "{\"error\":\"rate limited\"}");
        return;
    }

    int track = 1;
    if (req->hasParam("track")) {
        String val = req->getParam("track")->value();
        int parsed = val.toInt();
        if (parsed > 0 && parsed <= 999) track = parsed;
    }

    if (_scheduler) _scheduler->triggerPreview(track);
    req->send(200, "application/json", "{\"ok\":true,\"track\":" + String(track) + "}");
}

void LocalServer::_handleVolume(AsyncWebServerRequest* req) {
    if (!req->hasParam("level")) {
        req->send(400, "application/json", "{\"error\":\"missing level param\"}");
        return;
    }
    int vol = req->getParam("level")->value().toInt();
    vol = constrain(vol, 0, 30);

    _config->setVolume(vol);
    _config->save();
    if (_audio) _audio->setVolume(vol);

    req->send(200, "application/json", "{\"ok\":true,\"volume\":" + String(vol) + "}");
}

void LocalServer::_handleGetConfig(AsyncWebServerRequest* req) {
    String response;
    serializeJson(_config->getDoc(), response);
    req->send(200, "application/json", response);
}
