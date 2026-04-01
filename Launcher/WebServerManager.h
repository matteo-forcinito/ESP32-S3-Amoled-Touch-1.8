#pragma once
#include <WebServer.h>
#include <WiFi.h>
#include "SensorPCF85063.hpp"

extern SensorPCF85063 rtc;

class WebServerManager {
public:
    enum class Type {
        IDLE,
        AP,
        STA
    };

    enum class Status {
        IDLE,
        RUNNING,
        ERROR_WIFI_DISCONNECTED
    };

    static WebServerManager& get() {
        static WebServerManager instance;
        return instance;
    }

    void startAP(const String& ssid, const String& pwd);
    void startSTA();
    void stop();
    void loop();

    Status getStatus() const { return status; }
    Type getType() const { return type; }
    String getIP() const { return ipAddress; }

private:
    WebServerManager() : server(80) {}

    WebServer server;
    bool serverStarted = false;

    Type type = Type::IDLE;
    Status status = Status::IDLE;
    String ipAddress;

    String user = "admin";
    String pwd  = "admin";

    void setupServer();
    bool isLoggedIn();
};