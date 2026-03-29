#pragma once
#include "AppScreen.h"
#include "WifiManager.h"
#include "HomeScreen.h"

extern bool backHome;

class WiFiScreen : public AppScreen {
private:
    lv_obj_t *btnScan;
    lv_obj_t *btnStop;
    lv_obj_t *lblBtnScan;
    lv_obj_t *results;

    unsigned long connectingTime = 0;

    WifiManager::State lastState = WifiManager::State::IDLE;
public:
    void onCreate() override {
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(root, 10, 0);

        lv_obj_t *header = lv_obj_create(root);
        lv_obj_remove_style_all(header);
        lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
        lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);  // larghezza uguale allo schermo
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(header,
                              LV_FLEX_ALIGN_SPACE_BETWEEN,  // title a destra, battery a sinistra
                              LV_FLEX_ALIGN_CENTER,          // allinea verticalmente
                              LV_FLEX_ALIGN_CENTER);         // cross axis

        // Imposto flex row per contenitore
        lv_obj_t *lblTitle = lv_label_create(header);
        lv_label_set_text(lblTitle, "WiFi");
        //lv_obj_remove_style_all(btnContainer);

        lv_obj_t *btnContainer = lv_obj_create(header);
        lv_obj_remove_style_all(btnContainer);
        btnScan = lv_btn_create(btnContainer);
        lv_obj_add_event_cb(btnScan, [](lv_event_t *e) {
            if(WifiManager::getState() != WifiManager::State::SCANNING) {
                WifiManager::scanAsync();
            }
        }, LV_EVENT_CLICKED, NULL);
        lblBtnScan = lv_label_create(btnScan);
        lv_label_set_text(lblBtnScan, "Scan");

        btnContainer = lv_obj_create(header);
        lv_obj_remove_style_all(btnContainer);
        btnStop = lv_btn_create(btnContainer);
        lv_obj_add_event_cb(btnStop, [](lv_event_t *e) {
            WifiManager::stop();
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lblBtnStop = lv_label_create(btnStop);
        lv_label_set_text(lblBtnStop, "Disconnect");

        results = lv_obj_create(root);
        lv_obj_remove_style_all(results);
        lv_obj_set_flex_grow(results, 1);
        lv_obj_set_width(results, lv_pct(100));

        lv_obj_t *lblResults = lv_label_create(results);
        lv_label_set_text(lblResults, "No scan yet.");

        WifiManager::begin();
    }

    void onLoop() override {
        WifiManager::State state = WifiManager::getState();
        if(lastState == state) {
            if(state == WifiManager::State::CONNECTING) {
                if(WiFi.status() == WL_CONNECTED) {
                    WifiManager::connected();
                } else if(millis() - connectingTime > 6000) {
                    WifiManager::connectionError();
                }
            }

            return;
        }

        switch(state) {
            case WifiManager::State::SCANNING: {
                lv_obj_clean(results);
                lv_obj_t *loaderContainer = lv_obj_create(results);
                lv_obj_remove_style_all(loaderContainer);
                lv_obj_set_flex_flow(loaderContainer, LV_FLEX_FLOW_COLUMN);
                lv_obj_t *loader = lv_spinner_create(loaderContainer, 1000, 60);
                lv_obj_center(loader);
                lv_obj_t *lblLoader = lv_label_create(loaderContainer);
                lv_label_set_text(lblLoader, "Scanning..");

            break;
            }

            case WifiManager::State::SCANNED: {
                lv_obj_clean(results);
                lv_obj_set_flex_flow(results, LV_FLEX_FLOW_COLUMN);
                const std::vector<WifiNetwork> &networks = WifiManager::getNetworks();
                if(networks.size() < 1) {
                    lv_obj_t *lblNoResults = lv_label_create(results);
                    lv_label_set_text(lblNoResults, "No networks found.");

                    return;
                }
                lv_obj_t *lblNetworkSize = lv_label_create(results);
                lv_label_set_text_fmt(lblNetworkSize, "found %d networks", networks.size());
                for(const WifiNetwork &network : networks) {
                    lv_obj_t *networkContainer = lv_obj_create(results);
                    lv_obj_set_flex_flow(networkContainer, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_size(networkContainer, lv_pct(100), LV_SIZE_CONTENT);

                    lv_obj_t *lblTitle = lv_label_create(networkContainer);
                    lv_label_set_text(lblTitle, network.ssid.c_str());

                    lv_obj_t *lblRssi = lv_label_create(networkContainer);
                    lv_label_set_text(lblRssi, String(network.rssi).c_str());

                    if(network.saved) {
                        lv_obj_t *lblSaved = lv_label_create(networkContainer);
                        lv_label_set_text(lblSaved, "SAVED");
                    }

                    lv_obj_add_event_cb(networkContainer, [](lv_event_t *e) {
                        WifiNetwork *network = (WifiNetwork*) lv_event_get_user_data(e);
                        WifiManager::connect(network->ssid, network->pwd);
                    }, LV_EVENT_CLICKED, (void*)&network);
                }
            break;
            }
            case WifiManager::State::CONNECTING: {
                lv_obj_clean(results);
                lv_obj_t *loaderContainer = lv_obj_create(results);
                lv_obj_remove_style_all(loaderContainer);
                lv_obj_set_flex_flow(loaderContainer, LV_FLEX_FLOW_COLUMN);
                lv_obj_t *loader = lv_spinner_create(loaderContainer, 1000, 60);
                lv_obj_center(loader);
                lv_obj_t *lblLoader = lv_label_create(loaderContainer);
                lv_label_set_text(lblLoader, "CONNECTING..");
                connectingTime = millis();

            break;
            }
            case WifiManager::State::CONNECTED: {
                lv_obj_clean(results);
                lv_obj_set_flex_flow(results, LV_FLEX_FLOW_COLUMN);
                const std::vector<WifiNetwork> &networks = WifiManager::getNetworks();
                WifiNetwork connectedNetwork = WifiManager::getConnected();
                if(!connectedNetwork.ssid) {
                    WifiManager::connectionError();
                    return;
                }
                if(networks.size() < 1) {
                    lv_obj_t *lblNoResults = lv_label_create(results);
                    lv_label_set_text(lblNoResults, "No networks found.");

                    return;
                }
                lv_obj_t *lblNetworkSize = lv_label_create(results);
                lv_label_set_text_fmt(lblNetworkSize, "found %d networks", networks.size());
                for(const WifiNetwork &network : networks) {
                    lv_obj_t *networkContainer = lv_obj_create(results);
                    lv_obj_set_flex_flow(networkContainer, LV_FLEX_FLOW_COLUMN);
                    lv_obj_set_size(networkContainer, lv_pct(100), LV_SIZE_CONTENT);

                    lv_obj_t *lblTitle = lv_label_create(networkContainer);
                    lv_label_set_text(lblTitle, network.ssid.c_str());

                    lv_obj_t *lblRssi = lv_label_create(networkContainer);
                    lv_label_set_text(lblRssi, String(network.rssi).c_str());

                    if(network.saved) {
                        lv_obj_t *lblSaved = lv_label_create(networkContainer);
                        lv_label_set_text(lblSaved, "SAVED");
                    }

                    if(network.ssid == connectedNetwork.ssid) {
                        lv_obj_t *lblConnected = lv_label_create(networkContainer);
                        lv_label_set_text(lblConnected, "Connected");
                    }

                    lv_obj_add_event_cb(networkContainer, [](lv_event_t *e) {
                        WifiNetwork *network = (WifiNetwork*) lv_event_get_user_data(e);
                        WifiManager::connect(network->ssid, network->pwd);
                    }, LV_EVENT_CLICKED, (void*)&network);
                }

            break;
            }

            case WifiManager::State::STOPPED: {
                //ScreenManager::get().changeScreen(new HomeScreen());
                backHome = true;
                break;
            }
        }
        lastState = state;
    }
};