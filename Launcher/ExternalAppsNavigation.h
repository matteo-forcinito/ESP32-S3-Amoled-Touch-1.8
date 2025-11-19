#pragma once
#include "Navigation.h"
#include "SDManager.h"
#include "AppInstallerScreen.h"

class ExternalAppsNavigation : public Navigation {
public:
    const char *getTitle() const override { return "External Apps"; }

    // Funzione statica per gestire i click
    static void onAppClick(lv_event_t *e) {
        String *appName = static_cast<String *>(lv_event_get_user_data(e));

        static const char *btns[] = {"Si", "No", NULL};
        String mboxDescr = "Vuoi aprire l'app " + *appName + "?";

        lv_obj_t *mbox = lv_msgbox_create(lv_layer_top(), "Conferma", mboxDescr.c_str(), btns, true);
        lv_obj_center(mbox);

        // allochiamo un piccolo contesto per la callback
        MboxContext *ctx = new MboxContext{mbox, new String(*appName)};
        lv_obj_add_event_cb(mbox, [](lv_event_t * e) {
            lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_VALUE_CHANGED) {
                MboxContext *ctx = static_cast<MboxContext *>(lv_event_get_user_data(e));
                const char *txt = lv_msgbox_get_active_btn_text(ctx->mbox);

                if (!txt) return;

                if (strcmp(txt, "Si") == 0) {
                    Serial.printf("Apro l'app: %s\n", ctx->appName->c_str());
                    ScreenManager::get().changeScreen(new AppInstallerScreen(ctx->appName->c_str()));
                }

                // pulizia
                lv_obj_del(ctx->mbox);
                delete ctx->appName;
                delete ctx;
            }
        }, LV_EVENT_VALUE_CHANGED, ctx);
    }


    std::vector<MenuItem> buildAppList() override {
        std::vector<MenuItem> appsList;

        if (!sdManager.isReady()) return appsList;

        auto apps = sdManager.listAppFolders("/apps");
        for (auto &appName : apps) {
          String path = "S:/apps/" + appName + "/icon.bin";
          appsList.emplace_back(appName, path, onAppClick);
        }

        return appsList;
    }
};
