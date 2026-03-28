#pragma once
#include "SDManager.h"
#include "AppScreen.h"
#include "MenuItem.h"

extern SDManager sdManager;

class FileExplorerScreen : public AppScreen {
private:
    String path;
    String name;

    // Callback statica per i file
    static void fileItemEvent(lv_event_t *e) {
        lv_obj_t *obj = lv_event_get_target(e);
        String path((const char*) lv_event_get_user_data(e));
        USBSerial.print("Opening ");
        USBSerial.println(path);
        if (path.length() < 1) return;

        // Cambia schermata con il nuovo path
        ScreenManager::get().changeScreen(new FileExplorerScreen(path));
    }

    // Ricava il "nome" finale del path
    static String extractName(const String &p) {
        if (p == "/") return "Home";

        int lastSlash = p.lastIndexOf('/');
        if (lastSlash < 0 || lastSlash == p.length() - 1) {
            // caso improbabile, ritorna tutto il path
            return p;
        }
        return p.substring(lastSlash + 1);
    }

    static void dummyCallback(lv_event_t *e) {
    // Non fa nulla, solo per permettere la visualizzazione
    }

public:
    // Costruttore
    FileExplorerScreen(const String &p, const String &n = "") : path(p) {
        name = n.length() ? n : extractName(p);
    }

    void onCreate() override {
        std::vector<MenuItem> appsList;

        std::vector<FileEntry> files = sdManager.listFolder(path.c_str());
        std::sort(files.begin(), files.end(), [](const FileEntry &a, const FileEntry &b) {
            if (a.isDirectory && !b.isDirectory) return true;   // directory prima
            if (!a.isDirectory && b.isDirectory) return false; // file dopo
            return a.name < b.name;                            // ordine alfabetico
        });

        if(path != "/") {
          // Ricava path padre
          int lastSlash = path.lastIndexOf('/');
          String parentPath;
          if (lastSlash <= 0) {
              parentPath = "/"; // torniamo alla root
          } else {
              parentPath = path.substring(0, lastSlash);
          }
          MenuItem backItem(parentPath.c_str(), "S:/assets/icons/directory-back.bin", fileItemEvent);
          backItem.userData = strdup(parentPath.c_str());
          appsList.push_back(backItem);
        }

        for (auto &file : files) {
          String icon = file.isDirectory ? "S:/assets/icons/directory.bin" : "S:/assets/icons/file.bin";
          String filePath = path;
          if (filePath != "/" && filePath.charAt(filePath.length() - 1) != '/')
              filePath += "/";
          if(filePath.charAt(0) != '/') {
            filePath = "/" + filePath;
          }
          filePath += file.name;

          USBSerial.print("file path: ");
          USBSerial.println(filePath.c_str());
 
          MenuItem item(file.name, icon,
              file.isDirectory ? fileItemEvent : dummyCallback);
          item.userData = strdup(filePath.c_str());
          appsList.push_back(item);
        }

        // Cambia schermata con la lista di file
        ScreenManager::get().changeScreen(new PaginatorScreen(name.c_str(), appsList));
    }
};