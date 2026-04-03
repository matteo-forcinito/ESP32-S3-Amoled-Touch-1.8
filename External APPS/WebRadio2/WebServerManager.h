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

    String startPage(String ip, String additionalCSS = "") {
        String page = generateHead(additionalCSS);
        page += "<body>";
        page += generateHeader(ip);

        return page;
    } 

    String endPage() {
        return "</body></html>";
    } 

    
  String generateHead(String additionalCSS = "") {
    String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    page += generateCSS();
    if(additionalCSS.length() > 0) {
        page += additionalCSS;
    }
    page += "<title>SmartBox</title></head>";

    return page;
  }

  String generateHeader(String ip) {
    String page = "";
    page += "<header class='container'>";
    page += "   <h1>Benvenuto!</h1>";
    page += "   <div class='navigation'>";
    page += "       <a href='/addNetwork'><button> Configure WiFi </button></a><br>";
    page += "       <a href='/addAlarm'><button> Configure Alarm </button></a><br>";
    page += "       <a href='/setTime'><button> Set Time </button></a><br>";
    page += "   </div>";
    page += "   <p>Il tuo IP: " + ip + "</p>";
    page += "</header>";

    return page;
  }

  String generateCSS() {
    return R"rawliteral(
      <style>
        body { background: darkgrey; font-size: 16px; padding: 1rem; }
        header { 
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            font-size: 16px; 
            padding: 1rem; 
            gap: 1rem;
        }
        .container {
            padding: 1rem;
            background: white;
            box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
            border-radius: 5px;
            margin: 1rem;
            display: flex;
            justify-content: center;
            align-items: center;
            flex-direction: column;
        }
        .navigation { display: flex; }
        button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px; border: none; }
      </style>
    )rawliteral";
  }
};