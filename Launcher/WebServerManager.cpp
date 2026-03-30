#include "WebServerManager.h"
#include "WifiManager.h"
#include "AlarmManager.h"

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
        page += "<a href='/addNetwork'> Configure WiFi </a></hr>";
        page += "<a href='/addAlarm'> Configure Alarm </a></hr>";

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

    // ------------------ Aggiungi pagina /addAlarm ------------------
    server.on("/addAlarm", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String page = "<h1>Aggiungi nuova sveglia</h1>";
        page += "<form method='POST' action='/saveAlarm'>";
        page += "Ora: <input type='number' name='hour' min='0' max='23'><br>";
        page += "Minuto: <input type='number' name='minute' min='0' max='59'><br>";
        page += "Titolo: <input type='text' name='title'><br>";
        page += "Descrizione: <input type='text' name='description'><br>";
        page += "<label>Abilitata: <input type='checkbox' name='enabled' checked></label><br>";
        page += "<button type='submit'>Salva</button>";
        page += "</form>";
        page += "<a href='/'>Torna alla home</a>";

        server.send(200, "text/html", page);
    });

    // ------------------ Salvataggio sveglia ------------------
    server.on("/saveAlarm", HTTP_POST, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        Alarm a;
        a.hour = server.arg("hour").toInt();
        a.minute = server.arg("minute").toInt();
        a.startEpoch = time(nullptr); // ora corrente
        a.recurrence = (uint8_t)Recurrence::DAYS; // per ora giornaliera
        a.interval = 1;
        a.title = server.arg("title");
        a.description = server.arg("description");
        a.enabled = server.arg("enabled") == "on";

        AlarmManager::add(a);

        String page = "<h1>Sveglia salvata!</h1>";
        page += "<p>" + a.title + " alle " + String(a.hour) + ":" + String(a.minute) + "</p>";
        page += "<a href='/'>Torna alla home</a>";

        server.send(200, "text/html", page);
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