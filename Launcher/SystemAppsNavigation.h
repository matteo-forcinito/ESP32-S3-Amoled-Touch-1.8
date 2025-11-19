#pragma once
#include "Navigation.h"

#include "ControlCenterScreen.h"
#include "SetTimeScreen.h"
#include "FlashScreen.h"
#include "SetTimeNavigation.h"
#include "MoodeScreen.h"

class SystemAppsNavigation : public Navigation {
public:
    const char *getTitle() const override { return "System Apps"; }


    static void onAppClick(lv_event_t *e, lv_event_cb_t originalCallback) {
        String *appName = static_cast<String *>(lv_event_get_user_data(e));

        static const char *btns[] = {"Si", "No", NULL};
        String mboxDescr = "Vuoi aprire l'app " + *appName + "?";

        lv_obj_t *mbox = lv_msgbox_create(lv_layer_top(), "Conferma", mboxDescr.c_str(), btns, true);
        lv_obj_center(mbox);

        // Creiamo il contesto con la callback originale
        MboxContextWithCallback *ctx = new MboxContextWithCallback{mbox, new String(*appName), originalCallback, e};

        lv_obj_add_event_cb(mbox, [](lv_event_t *ev) {
            lv_event_code_t code = lv_event_get_code(ev);
            if (code == LV_EVENT_VALUE_CHANGED) {
                MboxContextWithCallback *ctx = static_cast<MboxContextWithCallback *>(lv_event_get_user_data(ev));
                const char *txt = lv_msgbox_get_active_btn_text(ctx->mbox);
                if (!txt) return;

                if (strcmp(txt, "Si") == 0 && ctx->originalCallback) {
                    // Chiamiamo la callback originale 👇
                    ctx->originalCallback(ctx->originalEvent);
                }

                // Pulizia
                lv_obj_del(ctx->mbox);
                delete ctx->appName;
                delete ctx;
            }
        }, LV_EVENT_VALUE_CHANGED, ctx);
    }


    std::vector<MenuItem> buildAppList() override {
        std::vector<MenuItem> appsList;
        
        appsList.emplace_back("Control Panel", "S:/assets/icons/control.bin", [](lv_event_t *e) {
            SystemAppsNavigation::onAppClick(e, [](lv_event_t *e){
                ScreenManager::get().changeScreen(new ControlCenterScreen());
            });
        });

        appsList.emplace_back("Set Time", "S:/assets/icons/clock.bin", [](lv_event_t *e) {
            SystemAppsNavigation::onAppClick(e, [](lv_event_t *e){
                //ScreenManager::get().changeScreen(new SetTimeScreen());
                SetTimeNavigation stn;
                stn.open();
            });
        });

        appsList.emplace_back("Torcia", "S:/assets/icons/flashlight.bin", [](lv_event_t *e) {
            SystemAppsNavigation::onAppClick(e, [](lv_event_t *e){
                ScreenManager::get().changeScreen(new FlashScreen());
            });
        });


        appsList.emplace_back("moOde", "S:/assets/icons/moOde.bin", [](lv_event_t *e) {
            SystemAppsNavigation::onAppClick(e, [](lv_event_t *e){
                ScreenManager::get().changeScreen(new MoodeScreen());
            });
        });

        return appsList;
    }
};
