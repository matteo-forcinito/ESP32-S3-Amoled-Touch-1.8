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
                savedNetwork.rssi = net.rssi;
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

#include "WifiManager.h"
#include "SDManager.h"
#include <ArduinoJson.h>

// ---------------------- LOAD SAVED NETWORKS ----------------------
std::vector<WifiNetwork> WifiManager::loadSavedNetworks(const char* path) {
    std::vector<WifiNetwork> savedNetworks;

    SDManager sdManager;
    File file = sdManager.open(path);
    if (!file || !file.available()) {
        Serial.println("[WifiManager] File non trovato o vuoto");
        return savedNetworks;
    }

    // Legge tutto il file in una stringa
    String jsonStr;
    while(file.available()) {
        jsonStr += (char)file.read();
    }
    file.close();

    // JSON: array di oggetti
    DynamicJsonDocument doc(2048); // aumenta se servono più reti
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) {
        Serial.print("[WifiManager] Errore parsing JSON: ");
        Serial.println(error.c_str());
        return savedNetworks;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        WifiNetwork net;
        net.ssid      = obj["ssid"].as<String>();
        net.pwd       = obj["pwd"].as<String>();
        net.saved     = true;
        net.connected = false;
        savedNetworks.push_back(net);

        Serial.print("[WifiManager] Trovata rete salvata: ");
        Serial.println(net.ssid);
    }

    return savedNetworks;
}

// ---------------------- SAVE NETWORK ----------------------
bool WifiManager::saveNetwork(const char* path, const String& ssid, const String& pwd) {
    SDManager sdManager;
    std::vector<WifiNetwork> networks = loadSavedNetworks(path);

    // Aggiorna se già presente
    bool found = false;
    for (auto &net : networks) {
        if (net.ssid == ssid) {
            net.pwd = pwd;
            found = true;
            break;
        }
    }

    if (!found) {
        WifiNetwork newNet;
        newNet.ssid      = ssid;
        newNet.pwd       = pwd;
        newNet.saved     = true;
        newNet.connected = false;
        networks.push_back(newNet);
    }


    // Prepara JSON
    DynamicJsonDocument doc(2048 + networks.size() * 128);
    JsonArray arr = doc.to<JsonArray>();
    for (auto &net : networks) {
        JsonObject obj = arr.createNestedObject();
        obj["ssid"] = net.ssid;
        obj["pwd"]  = net.pwd;
    }

    // Scrive su SD
    File file = SD_MMC.open(path, FILE_WRITE); // crea/sovrascrive
    if (!file) {
        return false;
    }

    serializeJson(doc, file);
    file.close();

    return true;
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