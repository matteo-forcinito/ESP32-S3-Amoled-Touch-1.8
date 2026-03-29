#include "WebServerManager.h"
#include "WifiManager.h"

void WebServerManager::startAP(const String& ssid, const String& pwd) {
    if (serverStarted) return;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), pwd.c_str());

    ipAddress = WiFi.softAPIP().toString();
    type = Type::AP;

    setupServer();
}

void WebServerManager::startSTA() {
    if (serverStarted) return;

    if (WiFi.status() != WL_CONNECTED) {
        status = Status::ERROR_WIFI_DISCONNECTED;
        return;
    }

    ipAddress = WiFi.localIP().toString();
    type = Type::STA;

    setupServer();
}

void WebServerManager::setupServer() {
    server.on("/", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String page = "<h1>Benvenuto!</h1>";
        page += "<p>Il tuo IP: " + server.client().remoteIP().toString() + "</p>";
        page += "<a href='/addNetwork'> Configure WiFi </a>";

        server.send(200, "text/html", page);
    });

    server.on("/addNetwork", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        int n = WiFi.scanNetworks();

        String page = "<h1>Seleziona rete</h1>";
        page += "<form method='POST' action='/saveNetwork'>";
        page += "<select name='ssid'>";

        for (int i = 0; i < n; ++i) {
            page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }

        page += "</select>";
        page += "<input type='password' name='pwd'>";
        page += "<button type='submit'>Salva</button>";
        page += "</form>";

        server.send(200, "text/html", page);
    });

    server.on("/saveNetwork", HTTP_POST, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String ssid = server.arg("ssid");
        String pwd  = server.arg("pwd");

        WifiManager wm;
        bool ok = wm.saveNetwork("/networks.json", ssid, pwd);

        String page = "<h1>";
        page += ok ? "Rete salvata correttamente!" : "Errore nel salvataggio!";
        page += "</h1>";
        page += "<p>SSID: " + ssid + "</p>";
        page += "<a href='/'>Torna alla home</a>";

        server.send(200, "text/html", page);

        wm.connect(ssid, pwd);
    });

    server.begin();
    serverStarted = true;
    status = Status::RUNNING;
}

void WebServerManager::loop() {
    if (serverStarted) {
        server.handleClient();
    }
}

void WebServerManager::stop() {
    if (serverStarted) {
        server.close();
        serverStarted = false;
    }

    type = Type::IDLE;
    status = Status::IDLE;
}

bool WebServerManager::isLoggedIn() {
    return server.authenticate(user.c_str(), pwd.c_str());
}