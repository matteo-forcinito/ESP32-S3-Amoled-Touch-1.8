#pragma once
#include "AppScreen.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <vector>
#include <string>

// Tipi per la navigazione
enum ItemType {
    ITEM_ROOT,
    ITEM_SSD,
    ITEM_PLAYLISTS,
    ITEM_RADIOS,
    ITEM_FOLDER,
    ITEM_FILE,
    ITEM_QUEUE
};

class MoodeScreen;

// Contesto per ogni pulsante/list item
struct ItemBtnCtx {
    MoodeScreen *screen;
    String name;
    ItemType type;
    lv_obj_t *btn;
};

// Contesto per MsgBox
struct DialogContext {
    MoodeScreen *self;
    String name;
    ItemType type;
};

struct RadioInfo {
    String displayName;
    String streamUrl;
    String coverPath; // può essere locale o URL
};

std::vector<RadioInfo> radiosInfo;

class MoodeScreen : public AppScreen {
public:
    bool isConnected = false;
    bool wasConnected = false;
    WiFiClient mpdClient;
    unsigned long lastUpdate = 0;

    const char* MPD_HOST = "192.168.1.128";
    const int   MPD_PORT = 6600;

    bool wifiRequested = false;

    // --- LVGL UI elements
    lv_obj_t *labelStatus;
    lv_obj_t *btnPlay;
    lv_obj_t *btnStop;
    lv_obj_t *btnPrev;
    lv_obj_t *btnNext;
    lv_obj_t *btnBack;
    lv_obj_t *list;
    lv_obj_t *volSlider;

    // navigazione
    ItemType currentType = ITEM_ROOT;
    String currentPath = "/";

    // teniamo un vettore dei contesti per pulizia sicura
    std::vector<ItemBtnCtx*> listContexts;

    // --- Lifecycle
    void onCreate() override {
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(root, 5, 0);

        lv_obj_t *label = lv_label_create(root);
        lv_label_set_text(label, "moOde Player");
        lv_obj_center(label);

        labelStatus = lv_label_create(root);
        lv_obj_set_width(labelStatus, lv_pct(100)); 
        lv_label_set_long_mode(labelStatus, LV_LABEL_LONG_WRAP);
        lv_label_set_text(labelStatus, "In Attesa di connessione..");

        createUI();
    }

    void loop() override {
        if (!isConnected && !wifiRequested) {
            wifiRequested = true;
            Serial.println("Connessione Al WiFi");
            connectWiFi();
        }

        if (!wasConnected && WiFi.status() == WL_CONNECTED) {
            wasConnected = true;
            populateRootMenu();
        }

        unsigned long now = millis();
        if(lastUpdate == 0 || now - lastUpdate > 10000) { // ogni 2 secondi
            lastUpdate = now;
            updateCurrentTrack();
        }
    }

    // --- UI
    void createUI() {

        // Play/Stop buttons
        lv_obj_t *btnContainer = lv_obj_create(root);
        lv_obj_remove_style_all(btnContainer);
        lv_obj_set_width(btnContainer, lv_pct(100));
        lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btnContainer,
                              LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        btnPrev = lv_btn_create(btnContainer);
        lv_obj_add_event_cb(btnPrev, btn_prev_event, LV_EVENT_CLICKED, this);
        lv_obj_t *lblPrev = lv_label_create(btnPrev);
        lv_label_set_text(lblPrev, LV_SYMBOL_LEFT);

        btnPlay = lv_btn_create(btnContainer);
        lv_obj_add_event_cb(btnPlay, btn_play_event, LV_EVENT_CLICKED, this);
        lv_obj_t *lbl1 = lv_label_create(btnPlay);
        lv_label_set_text(lbl1, LV_SYMBOL_PLAY);

        btnStop = lv_btn_create(btnContainer);
        lv_obj_add_event_cb(btnStop, btn_pause_event, LV_EVENT_CLICKED, this);
        lv_obj_t *lbl2 = lv_label_create(btnStop);
        lv_label_set_text(lbl2, LV_SYMBOL_PAUSE);

        btnNext = lv_btn_create(btnContainer);
        lv_obj_add_event_cb(btnNext, btn_next_event, LV_EVENT_CLICKED, this);
        lv_obj_t *lblNext = lv_label_create(btnNext);
        lv_label_set_text(lblNext, LV_SYMBOL_RIGHT);

        // Volume slider
        lv_obj_t *volContainer = lv_obj_create(root);
        lv_obj_remove_style_all(volContainer);
        lv_obj_set_size(volContainer, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(volContainer, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(volContainer, 5, 0);

        volSlider = lv_slider_create(volContainer);
        lv_obj_set_style_pad_all(volSlider, 5, 0);
        lv_obj_set_flex_grow(volSlider, 1);
        lv_obj_set_height(volSlider, 20);
        lv_slider_set_range(volSlider, 0, 100);
        lv_obj_add_event_cb(volSlider, volume_released_event, LV_EVENT_RELEASED, this);

        lv_obj_t *volIcon = lv_label_create(volContainer);
        lv_obj_set_style_pad_all(volIcon, 5, 0);
        lv_label_set_text(volIcon, LV_SYMBOL_AUDIO);
        lv_obj_set_style_text_color(volIcon, lv_color_white(), 0);
        lv_obj_set_style_text_font(volIcon, &lv_font_montserrat_24, 0);


        lv_obj_t *topRow = lv_obj_create(root);
        lv_obj_remove_style_all(topRow);
        lv_obj_set_width(topRow, lv_pct(100));
        lv_obj_set_flex_flow(topRow, LV_FLEX_FLOW_ROW);

        btnBack = lv_btn_create(topRow);
        lv_obj_add_event_cb(btnBack, btn_back_event, LV_EVENT_CLICKED, this);
        lv_obj_t *lblBack = lv_label_create(btnBack);
        lv_label_set_text(lblBack, LV_SYMBOL_LEFT);
        lv_obj_add_flag(btnBack, LV_OBJ_FLAG_HIDDEN);

        // List
        list = lv_list_create(root);
        lv_obj_set_width(list, lv_pct(100));
        lv_obj_set_height(list, 200);
        lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *btnShutDown = lv_btn_create(root);
        lv_obj_add_event_cb(btnShutDown, btn_shutdown_event, LV_EVENT_CLICKED, this);
        lv_obj_t *lblbtnShutDown = lv_label_create(btnShutDown);
        lv_label_set_text(lblbtnShutDown, "Shutdown");
    }

    // --- MPD helpers
    bool mpdConnect() {
        if (mpdClient.connected()) return true;
        if (!mpdClient.connect(MPD_HOST, MPD_PORT)) return false;

        unsigned long start = millis();
        while (!mpdClient.available() && millis() - start < 1000) delay(10);
        while (mpdClient.available()) mpdClient.read(); // flush greeting
        return true;
    }

    void mpdSend(const String &cmd) {
        if (!mpdConnect()) {
            Serial.println("❌ Connessione a MPD fallita!");
            if (labelStatus) lv_label_set_text(labelStatus, "❌ Connessione MPD fallita!");
            return;
        }
        mpdClient.print(cmd + "\n");
        Serial.println("➡️ " + cmd);
    }

    std::vector<String> mpdList(const String &command, const String &key) {
        std::vector<String> items;
        if (!mpdConnect()) return items;

        mpdSend(command);
        delay(200);

        String response;
        while (mpdClient.available()) {
            response += (char)mpdClient.read();
        }

        String search = key + ": ";
        int index = 0;
        while ((index = response.indexOf(search, index)) != -1) {
            index += search.length();
            int end = response.indexOf("\n", index);
            if (end == -1) end = response.length();
            String value = response.substring(index, end);
            value.trim();
            if (value.length() > 0) items.push_back(value);
            index = end;
        }
        return items;
    }

    // --- List navigation
    void addListItem(const String &name, ItemType type, const char *symbol = LV_SYMBOL_DIRECTORY) {
        lv_obj_t *btn = lv_list_add_btn(list, symbol, name.c_str());
        ItemBtnCtx *ctx = new ItemBtnCtx{this, name, type, btn};
        listContexts.push_back(ctx);

        lv_obj_add_event_cb(btn, [](lv_event_t *e){
            ItemBtnCtx *ctx = (ItemBtnCtx*)lv_event_get_user_data(e);
            if(!ctx) return;
            ctx->screen->showActionDialog(ctx->name, ctx->type);
        }, LV_EVENT_CLICKED, ctx);
    }

    void clearListItems() {
        for (auto ctx : listContexts) delete ctx;
        listContexts.clear();
        lv_obj_clean(list);
    }

// --- List navigation aggiornato
void populateRootMenu() {
    clearListItems();
    currentType = ITEM_ROOT;
    currentPath = "/";
    lv_obj_add_flag(btnBack, LV_OBJ_FLAG_HIDDEN);

    addListItem("SSD", ITEM_SSD, LV_SYMBOL_DIRECTORY);
    addListItem("Radio", ITEM_RADIOS, LV_SYMBOL_AUDIO);
    addListItem("Playlist", ITEM_PLAYLISTS, LV_SYMBOL_PLAY);
    addListItem("Queue", ITEM_QUEUE, LV_SYMBOL_LOOP);
}

void populateFolder(const String &path) {
    clearListItems();
    lv_obj_clear_flag(btnBack, LV_OBJ_FLAG_HIDDEN);

    String cmdDirs = "lsinfo \"" + path + "\"";
    auto dirs = mpdList(cmdDirs, "directory");
    for (auto &d : dirs) addListItem(d, ITEM_FOLDER, LV_SYMBOL_DIRECTORY);

    auto files = mpdList(cmdDirs, "file");
    for (auto &f : files) addListItem(f, ITEM_FILE, LV_SYMBOL_AUDIO);

    currentType = ITEM_FOLDER;
    currentPath = path;
}

void populatePlaylists() {
    clearListItems();
    lv_obj_clear_flag(btnBack, LV_OBJ_FLAG_HIDDEN);

    auto pls = mpdList("listplaylists", "playlist");
    for (auto &p : pls) addListItem(p, ITEM_PLAYLISTS, LV_SYMBOL_PLAY);

    currentType = ITEM_PLAYLISTS;
    currentPath = "/";
}

void populatePlaylistFiles(const String &name) {
    clearListItems();
    lv_obj_clear_flag(btnBack, LV_OBJ_FLAG_HIDDEN);

    // Lista dei file della playlist
    auto files = mpdList("listplaylist \"" + name + "\"", "file");
    for (auto &f : files) addListItem(f, ITEM_FILE, LV_SYMBOL_AUDIO);

    currentType = ITEM_PLAYLISTS;
    currentPath = name;
}


void populateRadioFiles(const String &playlistName) {
    clearListItems();
    lv_obj_clear_flag(btnBack, LV_OBJ_FLAG_HIDDEN);
    Serial.println("=== Playlist: " + playlistName + " ===");
    auto lines = mpdList("listplaylist \"" + playlistName + "\"", "file");
    for(auto &line : lines) {
        Serial.println(line);
        String displayName = line;
        // se è una riga #EXTINF, prendiamo solo il nome
        if(line.startsWith("#EXTINF:")) {
            int comma = line.indexOf(',');
            if(comma > 0) displayName = line.substring(comma + 1);
        }
        addListItem(displayName, ITEM_FILE, LV_SYMBOL_AUDIO);
    }

    currentType = ITEM_RADIOS;
    currentPath = playlistName;
}

void populateQueue() {
    clearListItems();
    lv_obj_clear_flag(btnBack, LV_OBJ_FLAG_HIDDEN);

    Serial.println("=== Queue ===");

    // Lista dei file nella coda corrente
    auto queueFiles = mpdList("playlistinfo", "file"); // prende tutti i file in coda
    for (auto &f : queueFiles) {
        String displayName = f;

        // Se la riga contiene #EXTINF, usa il nome leggibile
        if (f.startsWith("#EXTINF:")) {
            int comma = f.indexOf(',');
            if (comma > 0) displayName = f.substring(comma + 1);
        }

        addListItem(displayName, ITEM_FILE, LV_SYMBOL_AUDIO);
        Serial.println(displayName);
    }

    currentType = ITEM_QUEUE; // puoi creare un nuovo tipo ITEM_QUEUE se vuoi
    currentPath = "queue";
}



void openItem(const String &name, ItemType type) {
    if(type == ITEM_SSD) populateFolder("/");
    else if(type == ITEM_FOLDER) populateFolder(name);
    else if(type == ITEM_PLAYLISTS) populatePlaylistFiles(name);
    else if(type == ITEM_RADIOS) populateRadioFiles(name);
    else if(type == ITEM_QUEUE) populateQueue();
    else if(type == ITEM_FILE) playItem(name, type);
}

void showActionDialog(const String &name, ItemType type) {
    static const char *btns[] = {"Apri cartella", "Riproduci", NULL};
    lv_obj_t *mbox = lv_msgbox_create(lv_layer_top(), "Azione", name.c_str(), btns, true);
    lv_obj_center(mbox);

    DialogContext *ctx = new DialogContext{this, name, type};

    lv_obj_add_event_cb(mbox, [](lv_event_t *ev){
        if(lv_event_get_code(ev) != LV_EVENT_VALUE_CHANGED) return;
        DialogContext *c = (DialogContext*)lv_event_get_user_data(ev);
        if(!c) return;

        lv_obj_t *mbox = lv_event_get_current_target(ev);
        const char *btn = lv_msgbox_get_active_btn_text(mbox);
        if(!btn) {
            lv_obj_del(mbox);
            delete c;
            return;
        }

        if(strcmp(btn, "Apri cartella") == 0) {
            // Solo se l'item ha senso come cartella
            if(c->type == ITEM_FOLDER || c->type == ITEM_SSD || c->type == ITEM_RADIOS || c->type == ITEM_PLAYLISTS || c->type == ITEM_QUEUE)
                c->self->openItem(c->name, c->type);
        } else if(strcmp(btn, "Riproduci") == 0) {
            c->self->playItem(c->name, c->type);
        }

        lv_obj_del(mbox);
        delete c;
    }, LV_EVENT_VALUE_CHANGED, ctx);
}

void updateCurrentTrack() {
    if(!mpdConnect()) return;

    // --- Leggi la canzone corrente
    mpdSend("currentsong");
    delay(50);

    String response;
    while(mpdClient.available()) response += (char)mpdClient.read();

    // Leggi il file/stream corrente
    String currentFile;
    int idx = response.indexOf("file: ");
    if(idx != -1) {
        int end = response.indexOf('\n', idx);
        currentFile = response.substring(idx + 6, end);
        currentFile.trim();
    }

    // --- Determina se sta suonando
    mpdSend("status");
    delay(50);

    String statusResp;
    while(mpdClient.available()) statusResp += (char)mpdClient.read();
    bool playing = statusResp.indexOf("state: play") != -1;

    // --- Nome visualizzato e copertina
    String trackName = "";
    String coverPath = "";

    bool foundRadio = false;
    for(auto &r : radiosInfo) {
        if(r.streamUrl == currentFile) {
            trackName = r.displayName;
            coverPath = r.coverPath;
            foundRadio = true;
            break;
        }
    }

    if(!foundRadio) {
        // Se non è radio, cerca il Title
        idx = response.indexOf("Title: ");
        if(idx != -1) {
            int end = response.indexOf('\n', idx);
            trackName = response.substring(idx + 7, end);
            trackName.trim();
        }

        if(trackName.length() == 0) {
            // fallback al nome file
            trackName = currentFile;
        }
        coverPath = ""; // copertina per file locali da implementare se vuoi
    }

    // --- Aggiorna label
    String labelText = trackName;
    lv_label_set_text(labelStatus, labelText.c_str());
}



    void playItem(const String &name, ItemType type) {
        mpdSend("stop");
        mpdSend("clear");

        if(type == ITEM_PLAYLISTS || type == ITEM_RADIOS) {
            mpdSend("load \"" + name + "\"");
        } 
        else if(type == ITEM_FOLDER) {
            // funzione ricorsiva per aggiungere tutti i file
            addFolderRecursive(name);
        } 
        else if(type == ITEM_FILE) {
            mpdSend("add \"" + name + "\"");
        } 
        else {
            mpdSend("add \"" + name + "\"");
        }

        mpdSend("play");
        //lv_label_set_text(labelStatus, ("▶️ " + name).c_str());
    }

    // Funzione helper ricorsiva
    void addFolderRecursive(const String &path) {
        // Aggiunge file nella cartella corrente
        auto files = mpdList("lsinfo \"" + path + "\"", "file");
        for(auto &f : files) {
            mpdSend("add \"" + f + "\"");
            delay(20); // piccolo delay per non saturare MPD
        }

        // Cerca eventuali sottocartelle
        auto dirs = mpdList("lsinfo \"" + path + "\"", "directory");
        for(auto &d : dirs) {
            addFolderRecursive(d); // ricorsione
        }
    }

    // --- LVGL Callbacks
    static void btn_shutdown_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        static const char *btns[] = {"Si", NULL};
        lv_obj_t *mbox = lv_msgbox_create(lv_layer_top(), "Conferma", "Vuoi spegnere moOde?", btns, true);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, [](lv_event_t *ev) {
            lv_obj_t *mbox = lv_event_get_current_target(ev);
            const char *btn = lv_msgbox_get_active_btn_text(mbox);
            if(strcmp(btn, "Si") == 0) {
                MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(ev);
                if(!self) return;
                self->shutdownMoode();
            }
        }, LV_EVENT_VALUE_CHANGED, self);
        self->mpdSend("previous");
    }

    static void btn_prev_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        self->mpdSend("previous");
    }
    static void btn_next_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        self->mpdSend("next");
    }

    static void btn_play_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        self->mpdSend("pause 0");
    }

    static void btn_pause_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        self->mpdSend("pause 1");
    }

    static void volume_released_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        lv_obj_t *slider = lv_event_get_target(e);
        int vol = lv_slider_get_value(slider);
        self->mpdSend("setvol " + String(vol));
    }

    static void btn_back_event(lv_event_t *e) {
        MoodeScreen *self = (MoodeScreen*)lv_event_get_user_data(e);
        if(self->currentType == ITEM_ROOT) return;
        else if(self->currentType == ITEM_FOLDER) {
            String p = self->currentPath;
            if(p == "/" || p.length() == 0) self->populateRootMenu();
            else {
                int idx = p.lastIndexOf('/');
                String parent = idx <= 0 ? "/" : p.substring(0, idx);
                self->populateFolder(parent);
            }
        } else self->populateRootMenu();
    }

    // --- WiFi
    bool connectWiFi() {
        WiFi.begin("WINDTRE-CEDF38 2.4GHz", "6djyuwd9mwf4sy9u");
        unsigned long start = millis();
        while(WiFi.status() != WL_CONNECTED && millis() - start < 6000) delay(200);

        if(WiFi.status() != WL_CONNECTED) {
            WiFi.begin("MERCUSYS_7635", "34067642");
            start = millis();
            while(WiFi.status() != WL_CONNECTED && millis() - start < 6000) delay(200);
        }

        if(WiFi.status() != WL_CONNECTED) return false;

        isConnected = true;
        return true;
    }

void shutdownMoode() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClient client;
    if (!client.connect("192.168.1.128", 80)) {
        Serial.println("Connessione fallita...");
        return;
    }

    client.println("GET /command/?cmd=shutdown HTTP/1.1");
    client.println("Host: 192.168.1.128");
    client.println("Connection: close");
    client.println();

    Serial.println("Shutdown inviato a moOde");
}


    void onDestroy() override {
        WiFi.disconnect(true);  // disattiva per risparmiare batteria
        WiFi.mode(WIFI_OFF);
    } 

};
