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

        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page += "<h1>Benvenuto!</h1>";
        page += "<div class='navigation'>";
        page += "   <a href='/addNetwork'><button> Configure WiFi </button></a><br>";
        page += "   <a href='/addAlarm'><button> Configure Alarm </button></a><br>";
        page += "   <a href='/setTime'><button> Set Time </button></a><br>";
        page += "</div>";
        page += "<p>Il tuo IP: " + server.client().remoteIP().toString() + "</p>";
        page += "</div><body>";

        server.send(200, "text/html", page);
    });

    server.on("/addNetwork", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        int n = WiFi.scanNetworks();
        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page += "<h1>Seleziona rete</h1>";
        page += "<form method='POST' action='/saveNetwork'>";
        page += "<select name='ssid'>";

        for (int i = 0; i < n; ++i) {
            page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }

        page += "</select>";
        page += "<input type='password' name='pwd'>";
        page += "<button type='submit'>Salva</button>";
        page += "</form>";
        page += "</div></body>";

        server.send(200, "text/html", page);
    });

    server.on("/saveNetwork", HTTP_POST, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String ssid = server.arg("ssid");
        String pwd  = server.arg("pwd");

        WifiManager wm;
        bool ok = wm.saveNetwork("/networks.json", ssid, pwd);

        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page += "<h1>";
        page += ok ? "Rete salvata correttamente!" : "Errore nel salvataggio!";
        page += "</h1>";
        page += "<p>SSID: " + ssid + "</p>";
        page += "<a href='/'>Torna alla home</a>";
        page += "</div></body>";

        server.send(200, "text/html", page);

        wm.connect(ssid, pwd);
    });

    // ------------------ Aggiungi pagina /addAlarm ------------------
    server.on("/addAlarm", [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();


        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page += "<h1>Aggiungi nuova sveglia</h1>";
        page += "<form method='POST' action='/saveAlarm'>";
        page += "Ora: <input type='number' name='hour' min='0' max='23'><br>";
        page += "Minuto: <input type='number' name='minute' min='0' max='59'><br>";
        page += "Titolo: <input type='text' name='title'><br>";
        page += "Descrizione: <input type='text' name='description'><br>";
        page += "<label>Abilitata: <input type='checkbox' name='enabled' checked></label><br>";
        page += "<button type='submit'>Salva</button>";
        page += "</form>";
        page += "<a href='/'><button>Torna alla home</button></a>";
        page += "</div></body>";

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

        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page = "<h1>Sveglia salvata!</h1>";
        page += "<p>" + a.title + " alle " + String(a.hour) + ":" + String(a.minute) + "</p>";
        page += "<a href='/'><button>Torna alla home</button></a>";
        page += "</div></body>";

        server.send(200, "text/html", page);
    });

    server.on("/setTime", HTTP_GET, [this]() {
        if (!isLoggedIn()) return server.requestAuthentication();

        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page += "<h1>Configura ora</h1>";
        page += "<form method='POST' action='/saveTime'>";
        page += "Anno: <input type='number' name='year' id='year'><br>";
        page += "Mese: <input type='number' name='month' id='month'><br>";
        page += "Giorno: <input type='number' name='day' id='day'><br>";
        page += "Ora: <input type='number' name='hour' id='hour'><br>";
        page += "Minuto: <input type='number' name='minute' id='minute'><br>";
        page += "Secondo: <input type='number' name='second' id='second'><br>";
        page += "<button type='submit'>Salva</button>";
        page += "</form>";
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

        page += "</div></body>";

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


        String css = R"rawliteral(
            <head>
            <title>SmartBox</title>
            <style>
                body { display: flex; flex-direction: column; align-items: center; background: darkgrey; font-size: 16px; padding: 1rem; }
                .container {
                    padding: 1rem;
                    background: white;
                    box-shadow: 2px 2px 2px 2px rgba(0, 0, 0, .4);
                    border-radius: 5px;
                }
                .navigation { display: flex; }
                button { background: darkblue; color: white; padding: 0.5rem 1rem; border-radius: 5px;  }
            </style>
            </head>
        )rawliteral";
        String page = css;
        page += "<body><div class='container'>";
        page += "<h1>Ora impostata!</h1>";
        page += "<p>" + String(day) + "/" + String(month) + "/" + String(year) + " " +
                String(h) + ":" + String(m) + ":" + String(s) + "</p>";
        page += "<a href='/'><button>Torna alla home</button></a>";
        page += "</div></body>";

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