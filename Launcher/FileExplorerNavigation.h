#pragma once
#include "SDManager.h"
#include "Navigation.h"

extern SDManager sdManager;

class FileExplorerNavigation : public Navigation {
public:
    const char *getTitle() const override { return "File Explorer"; }

    std::vector<MenuItem> buildAppList() override {
      std::vector<MenuItem> appsList;

      std::vector<FileEntry> files = sdManager.listFolder("/");
      for(auto &file : files) {
        String icon = file.isDirectory ? "S:/assets/icons/directory.bin" : "S:/assets/icons/file.bin"; 
        appsList.emplace_back(file.name, icon, [](lv_event_t *e) {

        });
      }

      return appsList;
    }
};