#pragma once
#include <vector>
#include <ArduinoJson.h>
#include "SDManager.h"
#include "Utils.h"

class AlarmManager {
private:
    static std::vector<Alarm> alarms;
    static const char* FILE_PATH;

public:
    static void load();
    static bool save();
    static const std::vector<Alarm>& getAll();
    static void add(const Alarm& alarm);
    static void remove(size_t index);
    static void toggle(size_t index);

    static bool shouldTrigger(const Alarm& a, time_t now);
    static uint32_t checkAlarms(time_t now);
    static bool isAlarmImminent(const Alarm& alarm, int minutesBefore);
    static Alarm* getById(uint32_t id);
    static bool removeById(uint32_t id);
    static bool toggleById(uint32_t id);
};