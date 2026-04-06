#pragma once

#include <Arduino.h>
#include <WiFi.h>

enum class WifiState {
    DISCONNECTED,
    BLE_PROVISIONING,
    CAPTIVE_PORTAL,
    CONNECTING,
    CONNECTED
};

class WifiProvisioner {
public:
    bool begin(const char* ssid, const char* password);
    void update();
    void startProvisioning();
    void resetCredentials();

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    WifiState getState() const { return _state; }
    const char* getIP() const;

    // Callbacks
    using OnConnectedCb = void (*)(void);
    using OnCredentialsCb = void (*)(const char* ssid, const char* password);
    void onConnected(OnConnectedCb cb) { _onConnected = cb; }
    void onCredentials(OnCredentialsCb cb) { _onCredentials = cb; }

private:
    WifiState _state = WifiState::DISCONNECTED;
    unsigned long _bleStartTime = 0;
    bool _bleActive = false;
    bool _portalActive = false;
    char _apName[32];
    char _ipBuf[16];

    OnConnectedCb _onConnected = nullptr;
    OnCredentialsCb _onCredentials = nullptr;

    void _startBLE();
    void _stopBLE();
    void _startCaptivePortal();
    void _connectWifi(const char* ssid, const char* password);
    void _generateAPName();
};
