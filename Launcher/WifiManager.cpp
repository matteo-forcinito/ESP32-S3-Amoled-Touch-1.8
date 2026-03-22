#include "WifiManager.h"
#include <ArduinoJson.h>

volatile WifiManager::State WifiManager::state = WifiManager::State::IDLE;
std::vector<WifiNetwork> WifiManager::networks;
std::vector<WifiNetwork> WifiManager::savedNetworks;

void WifiManager::begin() {
    WiFi.mode(WIFI_STA);

    if(WiFi.status() == WL_CONNECTED) {
        state = State::CONNECTED;
    } else {
        state = State::IDLE;
    }
    savedNetworks = loadSavedNetworks("/networks.json");
}

void WifiManager::scanAsync() {

    if(state == State::SCANNING)
        return;
    
    clear();

    state = State::SCANNING;

    xTaskCreatePinnedToCore(
        scanTask,
        "wifiScan",
        4096,
        NULL,
        1,
        NULL,
        0
    );
}

void WifiManager::scanTask(void* param) {

    networks.clear();

    int n = WiFi.scanNetworks();

    if(n < 0) {
        state = State::ERROR;
        vTaskDelete(NULL);
        return;
    }
    for(int i = 0; i < n; i++) {
        WifiNetwork net;
        net.ssid = WiFi.SSID(i);
        net.rssi = WiFi.RSSI(i);
        WifiNetwork savedNetwork = WifiNetwork();
        bool found = false;
        for(auto &savedNet : savedNetworks) {
            if(savedNet.ssid == net.ssid) {
                savedNetwork = savedNet;
                savedNet.rssi = net.rssi;
                found = true;
                break;
            }
        }
        if(found) {
            networks.push_back(savedNetwork);
        } else {
            net.saved = false; // puoi integrare con SD
            networks.push_back(net);
        }
    }

    state = State::SCANNED;

    vTaskDelete(NULL);
}

WifiManager::State WifiManager::getState() {
    if(WiFi.status() == WL_CONNECTED && (
        state == State::CONNECTING ||
        state == State::IDLE
    )) {
        state = State::CONNECTED;
    }

    return state;
}

void WifiManager::stop() {
    WiFi.disconnect(true);   // disconnette e cancella eventuali credenziali temporanee
    WiFi.mode(WIFI_OFF);     // spegne la radio
    state = State::STOPPED;     // reset stato
}

const std::vector<WifiNetwork>& WifiManager::getNetworks() {
    return networks;
}

std::vector<WifiNetwork> WifiManager::loadSavedNetworks(const char* path) {
    std::vector<WifiNetwork> savedNetworks;

    SDManager sdManager;
    File file = sdManager.open(path);
    if(!file) {
        Serial.println("File non trovato");
        return savedNetworks;
    }

    // Legge tutto il file in una stringa
    String jsonStr;
    while(file.available()) {
        jsonStr += (char)file.read();
    }
    file.close();

    // Analizza il JSON
    DynamicJsonDocument doc(1024); // dimensione buffer: regola se serve più grande
    DeserializationError error = deserializeJson(doc, jsonStr);

    if(error) {
        Serial.print("Errore parsing JSON: ");
        Serial.println(error.c_str());
        return savedNetworks;
    }

    // JSON come array
    for(JsonObject obj : doc.as<JsonArray>()) {
        WifiNetwork net;
        net.ssid = obj["ssid"].as<String>();
        net.pwd  = obj["pwd"].as<String>();
        net.saved = true;
        net.connected = false;
        savedNetworks.push_back(net);

        Serial.print("SSID: ");
        Serial.println(net.ssid.c_str());
        Serial.print("PWD: ");
        Serial.println(net.pwd.c_str());
    }

    return savedNetworks;
}

WifiNetwork WifiManager::getConnected() {
    if(WiFi.status() == WL_CONNECTED) {
        for(auto &network : networks) {
            if(WiFi.SSID() == network.ssid) {
                network.connected = true;
                
                return network;
            }
        }
        WifiNetwork conn;
        conn.ssid = WiFi.SSID();
        conn.connected = true;

        return conn;
    }

    return WifiNetwork();
}

void WifiManager::connect(String ssid, String pwd) {
    WiFi.begin(ssid, pwd);

    state = State::CONNECTING;
}

void WifiManager::connected() {
    state = State::CONNECTED;
}

void WifiManager::connectionError() {
    state = State::ERROR;
}

void WifiManager::clear() {
    networks.clear();
    if(WiFi.status() == WL_CONNECTED) {
        state = State::CONNECTED;
        for(auto &network : networks) {
            if(WiFi.SSID() == network.ssid) {
                network.connected = true;
                //connected = network;
            }
        }
    } else {
        state = State::IDLE;
    }
}