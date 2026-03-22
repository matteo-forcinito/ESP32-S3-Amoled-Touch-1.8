#pragma once
#include <WiFi.h>
#include <vector>
#include "Utils.h"
#include "SDManager.h"

class WifiManager {
public:

    enum class State {
        IDLE,
        SCANNING,
        SCANNED,
        CONNECTED,
        CONNECTING,
        ERROR,
        STOPPED
    };

    static void begin();
    static void scanAsync();

    static void stop();

    static State getState();

    static void connected();
    static void connectionError();

    static const std::vector<WifiNetwork>& getNetworks();
    static  WifiNetwork getConnected();

    static void connect(String ssid, String pwd);

    static void clear();
    static std::vector<WifiNetwork> savedNetworks;
    
    static std::vector<WifiNetwork> loadSavedNetworks(const char* path);
private:
    static void scanTask(void* param);

    static volatile State state;
    static std::vector<WifiNetwork> networks;
};