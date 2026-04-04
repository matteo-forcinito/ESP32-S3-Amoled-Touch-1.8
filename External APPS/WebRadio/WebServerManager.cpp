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

        String page = startPage(server.client().remoteIP().toString());
        page += endPage();
        

        server.send(200, "text/html", page);
    });

    server.on("/addNetwork", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        int n = WiFi.scanNetworks();

        String page = startPage(server.client().remoteIP().toString());

        page += "<div class='container'>";
        page += "   <h1>Seleziona rete</h1>";
        page += "   <form method='POST' action='/saveNetwork'>";
        page += "       <select name='ssid'>";
        for (int i = 0; i < n; ++i) {
            page += "       <option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }
        page += "       </select>";
        page += "       <input type='password' name='pwd'>";
        page += "       <button type='submit'>Salva</button>";
        page += "   </form>";
        page += "</div>";
        
        page += endPage();

        server.send(200, "text/html", page);
    });

    server.on("/saveNetwork", HTTP_POST, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String ssid = server.arg("ssid");
        String pwd  = server.arg("pwd");

        WifiManager wm;
        bool ok = wm.saveNetwork("/networks.json", ssid, pwd);


        String page = startPage(server.client().remoteIP().toString());

        page += "<div class='container'>";
        page += "   <h1>";
        page += ok ? "Rete salvata correttamente!" : "Errore nel salvataggio!";
        page += "   </h1>";
        page += "   <p>SSID: " + ssid + "</p>";
        page += "   <a href='/'><button>Torna alla home</button></a>";
        page += "</div>";

        page += endPage();
        

        server.send(200, "text/html", page);

        wm.connect(ssid, pwd);
    });

    // ------------------ Aggiungi pagina /addAlarm ------------------
    server.on("/addAlarm", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String css = R"rawliteral(
            <style>
                .hours-container {
                    display: flex;
                    justify-content: center;
                    align-items: center;
                    gap: 1rem;
                    margin: 1rem;
                }
                .hours-container input {
                    width: 2.5ch;
                    align-self: normal;
                    font-size: 5rem;
                    text-align: center;
                }
                .hours-container .separator { font-size: 2rem; font-weight: bold; }
            </style>
        )rawliteral";

        String page = startPage(server.client().remoteIP().toString(), css);

        page += "<div class='container'>";
        page += "   <h1>Aggiungi nuova sveglia</h1>";
        page += "   <form method='POST' action='/saveAlarm'>";
        page += "       <div class='hours-container'>";
        page += "           <input type='number' name='hour' min='0' max='23'>";
        page += "           <p class='hours-separator'>:</p>";
        page += "           <input type='number' name='minute' min='0' max='59'>";
        page += "       </div>";
        page += "       <div class='input-container'>";
        page += "           <label for='title'>Titolo</label>";
        page += "           <input class='input' type='text' name='title'>";
        page += "       </div>";
        page += "       <div class='input-container'>";
        page += "           <label for='description'>Descrizione</label>";
        page += "           <input class='input' type='text' name='description'>";
        page += "       </div>";
        page += "       <div class='input-container'>";
        page += "           <label for='enabled'>Abilitata</label>";
        page += "           <input class='input checkbox' type='checkbox' name='enabled'>";
        page += "       </div>";
        page += "       <button type='submit'>Salva</button>";
        page += "   </form>";
        page += "   <a href='/'><button>Torna alla home</button></a>";
        page += "</div>";

        page += endPage();

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


        String page = startPage(server.client().remoteIP().toString());

        page += "<div class='container'>";
        page = "    <h1>Sveglia salvata!</h1>";
        page += "   <p>" + a.title + " alle " + String(a.hour) + ":" + String(a.minute) + "</p>";
        page += "   <a href='/'><button>Torna alla home</button></a>";
        page += "</div>";

        page += endPage();

        server.send(200, "text/html", page);
    });

    server.on("/setTime", HTTP_GET, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String page = startPage(server.client().remoteIP().toString());

        page += "<div class='container'>";
        page += "   <h1>Configura ora</h1>";
        page += "   <form method='POST' action='/saveTime'>";
        page += "       Anno: <input type='number' name='year' id='year'><br>";
        page += "       Mese: <input type='number' name='month' id='month'><br>";
        page += "       Giorno: <input type='number' name='day' id='day'><br>";
        page += "       Ora: <input type='number' name='hour' id='hour'><br>";
        page += "       Minuto: <input type='number' name='minute' id='minute'><br>";
        page += "       Secondo: <input type='number' name='second' id='second'><br>";
        page += "       <button type='submit'>Salva</button>";
        page += "   </form>";
        page += "<a href='/'><button>Torna alla home</button></a>";

        // Script per precompilare il form con l'ora del browser
        page += R"rawliteral(
            <script>
            const now = new Date();
            document.getElementById('year').value   = now.getFullYear();
            document.getElementById('month').value  = now.getMonth() + 1;
            document.getElementById('day').value    = now.getDate();
            document.getElementById('hour').value   = now.getHours();
            document.getElementById('minute').value = now.getMinutes();
            document.getElementById('second').value = now.getSeconds();
            </script>
        )rawliteral";

        page += "</div>";

        page += endPage();


        server.send(200, "text/html", page);
    });

    server.on("/saveTime", HTTP_POST, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        int h      = server.arg("hour").toInt();
        int m      = server.arg("minute").toInt();
        int s      = server.arg("second").toInt();
        int day    = server.arg("day").toInt();
        int month  = server.arg("month").toInt();
        int year   = server.arg("year").toInt();

        // Aggiorna RTC
        rtc.setDateTime(year, month, day, h, m, s);

        // Aggiorna anche l’orologio di sistema ESP32
        struct tm tm_info;
        tm_info.tm_year = year - 1900;
        tm_info.tm_mon  = month - 1;
        tm_info.tm_mday = day;
        tm_info.tm_hour = h;
        tm_info.tm_min  = m;
        tm_info.tm_sec  = s;
        time_t newTime = mktime(&tm_info);
        timeval tv = { .tv_sec = newTime, .tv_usec = 0 };
        settimeofday(&tv, nullptr);


        String page = startPage(server.client().remoteIP().toString());

        page += "<div class='container'>";
        page += "   <h1>Ora impostata!</h1>";
        page += "   <p>" + String(day) + "/" + String(month) + "/" + String(year) + " " +
                String(h) + ":" + String(m) + ":" + String(s) + "</p>";
        page += "   <a href='/'><button>Torna alla home</button></a>";
        page += "</div>";

        page += endPage();


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

    WifiManager::stop();
}

bool WebServerManager::isLoggedIn() {
    return server.authenticate(user.c_str(), pwd.c_str());
}