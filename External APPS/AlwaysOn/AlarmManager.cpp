#include "AlarmManager.h"
#include "FS.h"
#include "SD_MMC.h"

// Definizione variabili statiche
std::vector<Alarm> AlarmManager::alarms;
const char* AlarmManager::FILE_PATH = "/alarms.json";
uint32_t AlarmManager::id = 0;
Alarm AlarmManager::systemAlarms[] = {
    {1000001, 8, 30, 0, 0, 1, "Wake up", "System alarm", true},
    {1000002, 22, 0, 0, 0, 1, "Sleep", "System alarm", true},
    {1000002, 01, 05, 0, 0, 1, "Sleep", "System alarm", true},
    {1000002, 01, 10, 0, 0, 1, "Sleep", "System alarm", true}
};

const size_t AlarmManager::systemAlarmCount =
    sizeof(AlarmManager::systemAlarms) / sizeof(AlarmManager::systemAlarms[0]);

bool AlarmManager::prefsInitialized = false;
Preferences AlarmManager::prefs;

void AlarmManager::initPreferences() {
    if (!prefsInitialized) {
        prefs.begin("alarms", false);
        prefsInitialized = true;
    }
}

bool AlarmManager::isSystemAlarm(uint32_t id) {
    return id >= 1000000;
}

uint32_t AlarmManager::getLastId() {
    uint32_t maxId = 0;

    for (const auto& a : alarms) {
        if (a.id > maxId) maxId = a.id;
    }

    return maxId;
}
// --- LOAD ---
void AlarmManager::load() {
    for (size_t i = 0; i < systemAlarmCount; i++) {
        String key = "sys_" + String(systemAlarms[i].id);
        systemAlarms[i].enabled = prefs.getBool(key.c_str(), systemAlarms[i].enabled);
    }
    alarms.clear();
    SDManager sd;
    File file = sd.open(FILE_PATH);
    if (!file) return;

    String json;
    while (file.available()) json += (char)file.read();
    file.close();

    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, json)) return;

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        Alarm a;
        a.id = obj["id"] | 0;
        a.hour = obj["hour"] | 0;
        a.minute = obj["minute"] | 0;
        a.startEpoch = obj["start"] | 0;
        a.recurrence = obj["recurrence"] | 0;
        a.interval = obj["interval"] | 1;
        a.title = obj["title"] | "";
        a.description = obj["description"] | "";
        a.enabled = obj["enabled"] | true;
        alarms.push_back(a);
    }
}

// --- SAVE ---
bool AlarmManager::save() {
    SDManager sd;
    DynamicJsonDocument doc(2048 + alarms.size() * 128);
    JsonArray arr = doc.to<JsonArray>();

    for (auto &a : alarms) {
        JsonObject obj = arr.createNestedObject();
        
        obj["id"] = a.id;
        obj["hour"] = a.hour;
        obj["minute"] = a.minute;
        obj["start"] = a.startEpoch;
        obj["recurrence"] = a.recurrence;
        obj["interval"] = a.interval;
        obj["title"] = a.title;
        obj["description"] = a.description;
        obj["enabled"] = a.enabled;
    }

    File file = SD_MMC.open(FILE_PATH, FILE_WRITE);
    if (!file) return false;
    serializeJson(doc, file);
    file.close();

    return true;
}

// --- CRUD ---
const std::vector<Alarm>& AlarmManager::getAll() {
    return alarms;
}

void AlarmManager::add(const Alarm& alarm) {
    Alarm a = alarm;

    a.id = generateId();
    alarms.push_back(a);
    save();
}

void AlarmManager::remove(size_t index) {
    if (index >= alarms.size()) return;
    alarms.erase(alarms.begin() + index);
    save();
}

void AlarmManager::toggle(size_t index) {
    if (index >= alarms.size()) return;
    alarms[index].enabled = !alarms[index].enabled;
    save();
}

// --- CHECK ---
bool AlarmManager::shouldTrigger(const Alarm& a, time_t now) {
    if (!a.enabled) return false;

    RTC_DateTime datetime = rtc.getDateTime();

    return (datetime.hour == a.hour && datetime.minute == a.minute);
}

uint32_t AlarmManager::checkAlarms(time_t now) {
    // USER alarms
    for (const auto& a : alarms) {
        if (shouldTrigger(a, now)) return a.id;
    }

    // SYSTEM alarms
    for (size_t i = 0; i < systemAlarmCount; i++) {
        if (shouldTrigger(systemAlarms[i], now)) {
            return systemAlarms[i].id;
        }
    }

    return 0;
}

bool AlarmManager::isAlarmImminent(const Alarm& alarm, int minutesBefore) {
    if (!alarm.enabled) return false;
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    int nowMinutes = t->tm_hour * 60 + t->tm_min;
    int alarmMinutes = alarm.hour * 60 + alarm.minute;
    int diff = alarmMinutes - nowMinutes;
    if (diff < 0) diff += 24*60;
    return diff <= minutesBefore;
}

Alarm* AlarmManager::getById(uint32_t id) {
    // USER
    for (auto &a : alarms) {
        if (a.id == id) return &a;
    }

    // SYSTEM
    for (size_t i = 0; i < systemAlarmCount; i++) {
        if (systemAlarms[i].id == id) return &systemAlarms[i];
    }

    return nullptr;
}

bool AlarmManager::removeById(uint32_t id) {
    if (isSystemAlarm(id)) return false;
    for (auto it = alarms.begin(); it != alarms.end(); ++it) {
        if (it->id == id) {
            alarms.erase(it);
            save();
            return true;
        }
    }
    return false;
}

bool AlarmManager::edit(const Alarm& updated) {
    if (isSystemAlarm(updated.id)) return false;
    for (auto &a : alarms) {
        if (a.id == updated.id) {
            a = updated;   // sostituzione completa
            save();
            if(lastTriggeredId == a.id) {
                lastTriggeredId = 0;
            }
            return true;
        }
    }
    return false; // id non trovato
}

bool AlarmManager::toggleById(uint32_t id) {
    Alarm* a = getById(id);
    if (!a) return false;

    a->enabled = !a->enabled;

    if (isSystemAlarm(id)) {
        // salva su NVS
        String key = "sys_" + String(id);
        prefs.putBool(key.c_str(), a->enabled);
    } else {
        save(); // user alarms → SD
    }

    return true;
}

const Alarm& AlarmManager::getSystemAlarm(size_t index) {
    return systemAlarms[index];
}

size_t AlarmManager::getSystemAlarmCount() {
    return systemAlarmCount;
}