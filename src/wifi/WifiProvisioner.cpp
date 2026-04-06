#include "WifiProvisioner.h"
#include "../config/defaults.h"
#include <WiFiManager.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_mac.h>

// BLE UUIDs for WiFi provisioning service
#define WIFI_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SSID_CHAR_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define PASSWORD_CHAR_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STATUS_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26aa"

static WifiProvisioner* _instance = nullptr;
static String _bleSsid = "";
static String _blePassword = "";
static bool _bleCredentialsReceived = false;

// BLE Callbacks
class WifiProvCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) override {
        String uuid = pChar->getUUID().toString().c_str();
        String value = pChar->getValue().c_str();

        if (uuid == SSID_CHAR_UUID) {
            _bleSsid = value;
            Serial.printf("[BLE] SSID received: %s\n", _bleSsid.c_str());
        } else if (uuid == PASSWORD_CHAR_UUID) {
            _blePassword = value;
            Serial.println("[BLE] Password received");
            _bleCredentialsReceived = true;
        }
    }
};

void WifiProvisioner::_generateAPName() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(_apName, sizeof(_apName), "MyAthan-%02X%02X%02X", mac[3], mac[4], mac[5]);
}

bool WifiProvisioner::begin(const char* ssid, const char* password) {
    _instance = this;
    _generateAPName();

    // If we have credentials, try connecting directly
    if (ssid && strlen(ssid) > 0) {
        Serial.printf("[WiFi] Connecting to saved network: %s\n", ssid);
        _connectWifi(ssid, password);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            _state = WifiState::CONNECTED;
            Serial.printf("[WiFi] Connected! IP: %s\n", getIP());
            if (_onConnected) _onConnected();
            return true;
        }
        Serial.println("[WiFi] Connection failed, starting provisioning");
    }

    // No credentials or connection failed — start provisioning
    startProvisioning();
    return false;
}

void WifiProvisioner::update() {
    // Check if BLE credentials arrived
    if (_bleActive && _bleCredentialsReceived) {
        _bleCredentialsReceived = false;
        Serial.println("[WiFi] BLE credentials received, connecting...");
        _stopBLE();

        if (_onCredentials) {
            _onCredentials(_bleSsid.c_str(), _blePassword.c_str());
        }

        _connectWifi(_bleSsid.c_str(), _blePassword.c_str());
        _state = WifiState::CONNECTING;

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED) {
            _state = WifiState::CONNECTED;
            Serial.printf("[WiFi] Connected via BLE! IP: %s\n", getIP());
            if (_onConnected) _onConnected();
        } else {
            Serial.println("[WiFi] BLE credentials failed, falling back to portal");
            _startCaptivePortal();
        }
        return;
    }

    // BLE timeout → fall back to captive portal
    if (_bleActive && millis() - _bleStartTime >= BLE_ADVERTISING_TIMEOUT_MS) {
        Serial.println("[WiFi] BLE timeout, switching to captive portal");
        _stopBLE();
        _startCaptivePortal();
        return;
    }

    // Check connection status
    if (_state == WifiState::CONNECTED && WiFi.status() != WL_CONNECTED) {
        _state = WifiState::DISCONNECTED;
        Serial.println("[WiFi] Connection lost");
    }
}

void WifiProvisioner::startProvisioning() {
    _startBLE();
}

void WifiProvisioner::resetCredentials() {
    WiFi.disconnect(true, true);
    Serial.println("[WiFi] Credentials erased");
    startProvisioning();
}

const char* WifiProvisioner::getIP() const {
    IPAddress ip = WiFi.localIP();
    snprintf(const_cast<char*>(_ipBuf), sizeof(_ipBuf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return _ipBuf;
}

void WifiProvisioner::_startBLE() {
    _state = WifiState::BLE_PROVISIONING;
    _bleActive = true;
    _bleStartTime = millis();
    _bleCredentialsReceived = false;
    _bleSsid = "";
    _blePassword = "";

    BLEDevice::init(_apName);
    BLEServer* pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(WIFI_SERVICE_UUID);

    BLECharacteristic* ssidChar = pService->createCharacteristic(
        SSID_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    ssidChar->setCallbacks(new WifiProvCallbacks());

    BLECharacteristic* passChar = pService->createCharacteristic(
        PASSWORD_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    passChar->setCallbacks(new WifiProvCallbacks());

    BLECharacteristic* statusChar = pService->createCharacteristic(
        STATUS_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    statusChar->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(WIFI_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.printf("[BLE] Advertising as '%s' for %ds\n", _apName, BLE_ADVERTISING_TIMEOUT_MS / 1000);
}

void WifiProvisioner::_stopBLE() {
    if (!_bleActive) return;
    BLEDevice::deinit(true);
    _bleActive = false;
    Serial.println("[BLE] Stopped");
}

void WifiProvisioner::_startCaptivePortal() {
    _state = WifiState::CAPTIVE_PORTAL;
    _portalActive = true;

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);  // 3 minute timeout
    wm.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

    Serial.printf("[WiFi] Starting captive portal: %s\n", _apName);

    if (wm.startConfigPortal(_apName)) {
        _state = WifiState::CONNECTED;
        Serial.printf("[WiFi] Connected via portal! IP: %s\n", getIP());
        if (_onCredentials) {
            _onCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
        }
        if (_onConnected) _onConnected();
    } else {
        Serial.println("[WiFi] Portal timed out");
        _state = WifiState::DISCONNECTED;
    }
    _portalActive = false;
}

void WifiProvisioner::_connectWifi(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    _state = WifiState::CONNECTING;
}
