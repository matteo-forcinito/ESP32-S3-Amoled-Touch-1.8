#pragma once
#include "AppScreen.h"
#include "MenuItem.h"
#include <vector>
#include <string>
#include "SDManager.h"

#include "ControlCenterScreen.h"

extern SensorPCF85063 rtc;
extern XPowersPMU power; 
extern SDManager sdManager;

class PaginatorScreen : public AppScreen {
private:
    std::string title;
    std::vector<MenuItem> items;

    int currentPage = 0;
    int itemsPerPage = 6;

    lv_obj_t *titleLabel = nullptr;
    lv_obj_t *timeLabel = nullptr;
    lv_obj_t *battery = nullptr;
    lv_obj_t *container = nullptr;
    lv_obj_t *pageLabel = nullptr;
    lv_obj_t *btnPrev = nullptr;
    lv_obj_t *btnNext = nullptr;
    lv_obj_t *footer = nullptr;

    unsigned long lastUpdate = 0;

    lv_obj_t *connections;
    lv_obj_t *wifiConnection;

    bool wifiConnected = false;
public:
    PaginatorScreen(const std::string &title, std::vector<MenuItem> items)
      : title(title), items(std::move(items)) {}

    void onCreate() override {
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(root, 10, 0);
        lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);

        lv_obj_t *header = lv_obj_create(root);
        lv_obj_remove_style_all(header);
        lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
        lv_obj_set_size(header, 340, LV_SIZE_CONTENT);  // larghezza uguale allo schermo
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

        // Imposto flex row per contenitore
        lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(header,
                              LV_FLEX_ALIGN_SPACE_BETWEEN,  // title a destra, battery a sinistra
                              LV_FLEX_ALIGN_CENTER,          // allinea verticalmente
                              LV_FLEX_ALIGN_CENTER);         // cross axis
        lv_obj_set_style_pad_all(header, 10, 0); // padding interno

        lv_obj_add_event_cb(header, [](lv_event_t *e) {
          lv_dir_t dir = lv_indev_get_gesture_dir(lv_event_get_indev(e));
          //if (dir == LV_DIR_BOTTOM) {
              ScreenManager::get().changeScreen(new ControlCenterScreen());
              // Azione da eseguire
          //}
        }, LV_EVENT_CLICKED, NULL);

        // Label titolo
        
        timeLabel = lv_label_create(header);

        connections = lv_obj_create(header);
        lv_obj_remove_style_all(connections);
        lv_obj_set_flex_flow(connections, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_grow(connections, 1);
        lv_obj_set_height(connections, LV_SIZE_CONTENT);

        wifiConnection = lv_label_create(connections);
        lv_label_set_text(wifiConnection, LV_SYMBOL_WIFI);
        lv_obj_add_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);lv_obj_set_style_pad_left(wifiConnection, 8, 0);   // padding sinistro
        lv_obj_set_style_pad_right(wifiConnection, 8, 0);  // padding destro

        // Label batteria
        battery = lv_label_create(header);
        update();
        lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 10);

        // ---- TITOLO ----
        titleLabel = lv_label_create(root);
        lv_obj_set_width(titleLabel, lv_pct(100));
        lv_label_set_text(titleLabel, title.c_str());
        lv_obj_set_style_text_color(titleLabel, lv_color_white(), 0);
        lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_24, 0);
        lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_flex_align(titleLabel,
                              LV_FLEX_ALIGN_START,  // allinea gli elementi all'inizio in orizzontale
                              LV_FLEX_ALIGN_START,  // allinea all'inizio in verticale
                              LV_FLEX_ALIGN_START); // allinea se più righe
        lv_obj_set_style_pad_row(titleLabel, 10, 0);

        // ---- CONTAINER APP ----
        container = lv_obj_create(root);
        lv_obj_remove_style_all(container);
        lv_obj_set_width(container, lv_pct(100));
        lv_obj_set_flex_grow(container, 1);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);

        // Imposto layout flex
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW_WRAP);  // Row + wrap
        lv_obj_set_flex_align(container,
                              LV_FLEX_ALIGN_START,  // allinea gli elementi all'inizio in orizzontale
                              LV_FLEX_ALIGN_START,  // allinea all'inizio in verticale
                              LV_FLEX_ALIGN_START); // allinea se più righe

        lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(container, 0, 0);
        lv_obj_set_style_pad_row(container, 0, 0);
        lv_obj_set_style_pad_column(container, 0, 0);
        lv_obj_set_scroll_dir(container, LV_DIR_VER);

        // ---- FOOTER ----
        footer = lv_obj_create(root);
        lv_obj_remove_style_all(footer);
        lv_obj_set_width(footer, lv_pct(100));
        lv_obj_set_height(footer, LV_SIZE_CONTENT);
        //lv_obj_set_size(footer, lv_pct(100), lv_pct(10));
        lv_obj_set_style_pad_all(footer, 0, 0);
        lv_obj_set_style_pad_row(footer, 0, 0);
        lv_obj_set_style_pad_column(footer, 0, 0);
        lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN);

        btnPrev = createNavButton(footer, LV_SYMBOL_LEFT, [this](lv_event_t *) {
            if (currentPage > 0) {
                currentPage--;
                renderPage();
            }
        });

        pageLabel = lv_label_create(footer);
        lv_label_set_text(pageLabel, "");
        lv_obj_set_style_text_color(pageLabel, lv_color_white(), 0);

        btnNext = createNavButton(footer, LV_SYMBOL_RIGHT, [this](lv_event_t *) {
            if ((currentPage + 1) * itemsPerPage < (int)items.size()) {
                currentPage++;
                renderPage();
            }
        });

        renderPage();
    }

    void loop() override {
        RTC_DateTime datetime = rtc.getDateTime();
        int seconds = datetime.second;

        // Aggiorna all'inizio di ogni minuto
        if (seconds == 0 && (lastUpdate == 0 || millis() - lastUpdate >= 1000)) {
            update();
        }
        if(WiFi.getMode() != WIFI_MODE_NULL && !wifiConnected) {
            lv_obj_clear_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);
            wifiConnected = true;
        }
        if(WiFi.getMode() == WIFI_MODE_NULL && wifiConnected) {
            lv_obj_add_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);
            wifiConnected = false;
        }
    }

    void update() {
        RTC_DateTime datetime = rtc.getDateTime();

        lastUpdate = millis();

        char timeString[6];
        sprintf(timeString, "%02d:%02d",
                datetime.hour,
                datetime.minute);

        lv_label_set_text(timeLabel, timeString);

        int batteryPercent = power.getBatteryPercent();
        lv_label_set_text_fmt(battery, "%d%%", batteryPercent);
    }

private:
    // ---- Renderizza la pagina corrente ----
    void renderPage() {
        lv_obj_clean(container);

        int start = currentPage * itemsPerPage;
        int end = std::min(start + itemsPerPage, (int)items.size());

        if (items.size() <= itemsPerPage) {
            lv_obj_add_flag(footer, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(footer, LV_OBJ_FLAG_HIDDEN);

            // --- Pulsante "Next" ---
            if (end >= (int)items.size()) {
                lv_obj_set_style_opa(btnNext, LV_OPA_TRANSP, 0);        // invisibile
                lv_obj_add_flag(btnNext, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_CHECKABLE); // disattiva tutto
                lv_obj_clear_flag(btnNext, LV_OBJ_FLAG_CLICKABLE);       // non cliccabile
            } else {
                lv_obj_set_style_opa(btnNext, LV_OPA_COVER, 0);          // visibile
                lv_obj_add_flag(btnNext, LV_OBJ_FLAG_CLICKABLE);         // riattiva clic
            }

            // --- Pulsante "Prev" ---
            if (currentPage == 0) {
                lv_obj_set_style_opa(btnPrev, LV_OPA_TRANSP, 0);
                lv_obj_clear_flag(btnPrev, LV_OBJ_FLAG_CLICKABLE);
            } else {
                lv_obj_set_style_opa(btnPrev, LV_OPA_COVER, 0);
                lv_obj_add_flag(btnPrev, LV_OBJ_FLAG_CLICKABLE);
            }
        }


        for (int i = start; i < end; ++i) {
            auto &item = items[i];

            lv_obj_t *btn = lv_btn_create(container);
            lv_obj_remove_style_all(btn);
            lv_obj_set_width(btn, lv_pct(50));
            lv_obj_set_height(btn, LV_SIZE_CONTENT);
            lv_obj_set_style_radius(btn, 12, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E1E), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
            lv_obj_set_style_pad_all(btn, 8, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            if (item.callback) {
              String *nameCopy = new String(item.name); // <-- memoria valida finché non la delete
              lv_obj_add_event_cb(btn, item.callback, LV_EVENT_CLICKED, nameCopy);
            }
            lv_obj_t *icon;
            Serial.printf("[PaginatorScreen] icona: %s\n", item.icon);
            if (strncmp(item.icon.c_str(), "S:", 2) == 0) {
                // --- È un'immagine ---
                String iconPath = "S:/assets/icons/placeholder.bin";
                if(sdManager.isPathExists(item.icon.substring(2).c_str())) {
                  iconPath = item.icon;
                } 
                icon = lv_img_create(btn);
                lv_img_set_src(icon, iconPath.c_str());
                lv_obj_set_size(icon, 64, 64);
                lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);
            } else {
                // --- È un simbolo LVGL ---
                icon = lv_label_create(btn);
                lv_label_set_text(icon, item.icon.c_str());
                lv_obj_set_style_text_color(icon, lv_color_white(), 0);
                lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
                lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 5);
            }

            lv_obj_t *nameLbl = lv_label_create(btn);
            lv_label_set_text(nameLbl, item.name.c_str());
            lv_obj_set_style_text_color(nameLbl, lv_color_white(), 0);
            lv_obj_align(nameLbl, LV_ALIGN_BOTTOM_MID, 0, -5);

            //item.button = btn;
        }

        // Aggiorna label pagine
        int totalPages = (items.size() + itemsPerPage - 1) / itemsPerPage;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", currentPage + 1, totalPages);
        lv_label_set_text(pageLabel, buf);
    }

    // ---- Crea pulsanti di navigazione ----
    lv_obj_t *createNavButton(lv_obj_t *parent, const char *symbol,
                              std::function<void(lv_event_t *)> callback) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_height(btn, 40);
        lv_obj_set_flex_grow(btn, 1);
        //lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, symbol);
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

        // collega lambda a callback LVGL
        lv_obj_add_event_cb(
            btn,
            [](lv_event_t *e) {
                auto user_cb =
                    reinterpret_cast<std::function<void(lv_event_t *)> *>(
                        lv_event_get_user_data(e));
                if (user_cb)
                    (*user_cb)(e);
            },
            LV_EVENT_CLICKED,
            new std::function<void(lv_event_t *)>(callback));

        return btn;
    }
};
