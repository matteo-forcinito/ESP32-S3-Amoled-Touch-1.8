/* BTKeyboard - fixed layout + touch + safe callbacks
   ESP32-S3 Waveshare 1.8" (368x448)
*/

#include <Arduino.h>
#include <BleKeyboard.h>
#include <lvgl.h>
#include <Wire.h>

#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

// --- Display (SH8601 QSPI) ---
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// --- Touch FT3168 (I2C) ---
// Uses Arduino_FT3x68 class from Arduino_DriveBus_Library
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus;
std::unique_ptr<Arduino_FT3x68> FT3168;

// small LVGL draw buffer to save RAM
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 20];

#define LVGL_TICK_PERIOD_MS 2

// --- BLE ---
BleKeyboard bleKeyboard("ESP32-BTKeyboard", "Waveshare", 100);
volatile bool bleConnected = false;
bool keyboardCreated = false;
bool keyboardNeedsUpdate = false;

// --- LVGL objects ---
lv_obj_t *statusLabel = nullptr;
lv_obj_t *keyboardContainer = nullptr;

// --- Key data (global) ---
const char *letters[] = {
  "q","w","e","r","t","y","u","i","o","p",
  "a","s","d","f","g","h","j","k","l",
  "z","x","c","v","b","n","m"
};
const int lettersCount = sizeof(letters)/sizeof(letters[0]);

const char *numbers[] = {"1","2","3","4","5","6","7","8","9","0"};
const int numbersCount = sizeof(numbers)/sizeof(numbers[0]);

const char *symbols[] = {
  "!", "\"", "£", "$", "%", "&", "/", "(", ")", "=", 
  "?", "^", "_", "-", "+", "*", "@", "#", "§", "°",
  "[", "]", "{", "}", "<", ">", ",", ".", ";", ":"
};
const int symbolsCount = sizeof(symbols) / sizeof(symbols[0]);

enum KeyboardMode { MODE_LETTERS, MODE_NUMBERS, MODE_SYMBOLS };
KeyboardMode currentMode = MODE_LETTERS;
bool capsLock = false;

// --- LVGL tick ---
static void lv_tick_task(void *arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); }

// --- LVGL display flush ---
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}

// --- Touch read for FT3168 (returns coordinates in LVGL space) ---
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (!FT3168) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
    int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
    // clamp and map if necessary
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > LCD_WIDTH) x = LCD_WIDTH;
    if (y > LCD_HEIGHT) y = LCD_HEIGHT;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// --- LVGL callbacks (safe, no captures) ---
void letter_event_cb(lv_event_t *e) {
  const char *key = (const char*)lv_event_get_user_data(e);
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (!key || strlen(key) == 0) return;
  if (!bleKeyboard.isConnected()) return;

  // Mappa simboli manuale per layout italiano
  if (currentMode == MODE_SYMBOLS) {
    String k = key;
    char toSend = 0;

    if      (k == "!") toSend = '!';
    else if (k == "\"") toSend = '"';
    else if (k == "£") toSend = '£';
    else if (k == "$") toSend = '$';
    else if (k == "%") toSend = '%';
    else if (k == "&") toSend = '&';
    else if (k == "/") toSend = '/';
    else if (k == "(") toSend = '(';
    else if (k == ")") toSend = ')';
    else if (k == "=") toSend = '=';
    else if (k == "?") toSend = '?';
    else if (k == "^") toSend = '^';
    else if (k == "_") toSend = '_';
    else if (k == "-") toSend = '-';
    else if (k == "+") toSend = '+';
    else if (k == "*") toSend = '*';
    else if (k == "@") toSend = '@';
    else if (k == "#") toSend = '#';
    else if (k == "§") toSend = '§';
    else if (k == "°") toSend = '°';
    else if (k == "[") toSend = '[';
    else if (k == "]") toSend = ']';
    else if (k == "{") toSend = '{';
    else if (k == "}") toSend = '}';
    else if (k == "<") toSend = '<';
    else if (k == ">") toSend = '>';
    else if (k == ",") toSend = ',';
    else if (k == ".") toSend = '.';
    else if (k == ";") toSend = ';';
    else if (k == ":") toSend = ':';

    if (toSend) {
      bleKeyboard.print(String(toSend));
    }
    return;
  }

  // Modalità lettere/numeri normale
  String k = key;
  if (capsLock && k.length() == 1 && isalpha(k[0])) {
    k[0] = toupper(k[0]);
  }
  bleKeyboard.print(k);
}


void special_event_cb(lv_event_t *e) {
  const char *label = (const char*)lv_event_get_user_data(e);
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  Serial.printf("Special key: %s\n", label);
  if (!bleKeyboard.isConnected() && strcmp(label, "SPACE") != 0) {
    // SPACE possiamo lasciare che venga premuta anche se non connessi (opzionale)
    return;
  }

  if (strcmp(label, "DEL") == 0) {
    bleKeyboard.write(KEY_BACKSPACE);
    return;
  }

  if (strcmp(label, "SPACE") == 0) {
    bleKeyboard.print(" ");
    return;
  }

  if (strcmp(label, "CAPS") == 0) {
    // toggle caps lock
    capsLock = !capsLock;
    keyboardNeedsUpdate = true;
    return;
  }

  if (strcmp(label, "123") == 0) {
    // se già in NUMBERS -> torna a LETTERS, altrimenti passa a NUMBERS
    if (currentMode == MODE_NUMBERS) currentMode = MODE_LETTERS;
    else currentMode = MODE_NUMBERS;
    keyboardNeedsUpdate = true;
    return;
  }

  if (strcmp(label, "#+=") == 0) {
    // toggle simboli
    if (currentMode == MODE_SYMBOLS) currentMode = MODE_LETTERS;
    else currentMode = MODE_SYMBOLS;
    keyboardNeedsUpdate = true;
    return;
  }

  if (strcmp(label, "ABC") == 0) {
    // for completeness: force letters
    currentMode = MODE_LETTERS;
    keyboardNeedsUpdate = true;
    return;
  }
}

// --- UI create / destroy ---
void clearKeyboard() {
  if (keyboardContainer) {
    lv_obj_del(keyboardContainer);
    keyboardContainer = nullptr;
  }
}

void createKeyboard() {
  clearKeyboard();
  keyboardContainer = lv_obj_create(lv_scr_act());
  lv_obj_set_size(keyboardContainer, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_clear_flag(keyboardContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(keyboardContainer, lv_color_black(), 0);

  // Layout params
  const int btn_w = 44;
  const int btn_h = 44;
  const int min_spacing_x = 6;
  const int min_spacing_y = 8;
  const int margin_lr = 8;
  const int topMargin = 48;
  const int bottomReserved = 96; // spazio riservato per space + corner buttons

  // Attiva i tasti secondo la modalità
  const char **activeKeys;
  int keyCount;
  if (currentMode == MODE_LETTERS) { activeKeys = letters; keyCount = lettersCount; }
  else if (currentMode == MODE_NUMBERS) { activeKeys = numbers; keyCount = numbersCount; }
  else { activeKeys = symbols; keyCount = symbolsCount; }

  // Calcolo colonne massime effettive che entrano orizzontalmente
  int availableW = LCD_WIDTH - margin_lr * 2;
  int maxCols = min(10, keyCount);
  int cols = maxCols;
  int spacingX = min_spacing_x;
  while (cols > 1) {
    spacingX = (availableW - cols * btn_w) / (cols + 1);
    if (spacingX >= min_spacing_x) break;
    cols--;
  }
  if (cols < 1) cols = 1;

  // righe necessarie
  int rows = (keyCount + cols - 1) / cols;

  // Calcolo startX per centrare le righe (utilizzeremo rowStartX dinamico per righe corte)
  int totalBtnsWidth = cols * btn_w + (cols - 1) * spacingX;
  int globalStartX = margin_lr + (availableW - totalBtnsWidth) / 2;

  // Vertical spacing che faccia stare tutto nello spazio disponibile (topMargin..LCD_HEIGHT-bottomReserved)
  int usableH = LCD_HEIGHT - topMargin - bottomReserved;
  int totalBtnsH = rows * btn_h;
  int spacingY;
  if (rows > 1) spacingY = max(min_spacing_y, (usableH - totalBtnsH) / (rows + 1));
  else spacingY = min_spacing_y;
  int startY = topMargin + spacingY;

  // Posizionamento riga per riga, centrando eventuali righe corte
  int idx = 0;
  for (int r = 0; r < rows && idx < keyCount; ++r) {
    int remaining = keyCount - r * cols;
    int rowCols = min(cols, remaining);
    int rowTotalWidth = rowCols * btn_w + (rowCols - 1) * spacingX;
    int rowStartX = (LCD_WIDTH - rowTotalWidth) / 2;

    int y = startY + r * (btn_h + spacingY);

    for (int c = 0; c < rowCols && idx < keyCount; ++c, ++idx) {
      int x = rowStartX + c * (btn_w + spacingX);

      // sicurezza: clamp
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x + btn_w > LCD_WIDTH) x = LCD_WIDTH - btn_w;
      if (y + btn_h > LCD_HEIGHT - bottomReserved) y = LCD_HEIGHT - bottomReserved - btn_h;

      lv_obj_t *btn = lv_btn_create(keyboardContainer);
      lv_obj_set_size(btn, btn_w, btn_h);
      lv_obj_set_pos(btn, x, y);

      lv_obj_t *lbl = lv_label_create(btn);
      // testo con caps se necessario (letters only)
      if (capsLock && currentMode == MODE_LETTERS && strlen(activeKeys[idx]) == 1) {
        char temp[2] = { (char)toupper(activeKeys[idx][0]), 0 };
        lv_label_set_text(lbl, temp);
        // passiamo comunque l'originale come user_data per coerenza
        lv_obj_add_event_cb(btn, letter_event_cb, LV_EVENT_CLICKED, (void*)activeKeys[idx]);
      } else {
        lv_label_set_text(lbl, activeKeys[idx]);
        lv_obj_add_event_cb(btn, letter_event_cb, LV_EVENT_CLICKED, (void*)activeKeys[idx]);
      }
      lv_obj_center(lbl);
    }
  }

  // --- Spacebar: posizionato sopra bottomReserved con margine ---
  // --- Spacebar ---
  const int space_h = 48;
  const int space_w = LCD_WIDTH - 180; // accorciata per lasciare spazio ai pulsanti laterali
  const int space_x = (LCD_WIDTH - space_w) / 2;
  const int space_y = LCD_HEIGHT - bottomReserved + 8;

  lv_obj_t *space = lv_btn_create(keyboardContainer);
  lv_obj_set_size(space, space_w, space_h);
  lv_obj_set_pos(space, space_x, space_y);

  lv_obj_t *sLbl = lv_label_create(space);
  lv_label_set_text(sLbl, "SPACE");
  lv_obj_center(sLbl);
  lv_obj_add_event_cb(space, special_event_cb, LV_EVENT_CLICKED, (void*)"SPACE");

  // --- Corner buttons ---
  lv_obj_t *capsBtn = lv_btn_create(keyboardContainer);
  lv_obj_set_size(capsBtn, 64, 44);
  lv_obj_align(capsBtn, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_obj_t *capsLbl = lv_label_create(capsBtn);
  lv_label_set_text(capsLbl, "CAPS");
  lv_obj_center(capsLbl);
  lv_obj_add_event_cb(capsBtn, special_event_cb, LV_EVENT_CLICKED, (void*)"CAPS");

  lv_obj_t *delBtn = lv_btn_create(keyboardContainer);
  lv_obj_set_size(delBtn, 64, 44);
  lv_obj_align(delBtn, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_t *delLbl = lv_label_create(delBtn);
  lv_label_set_text(delLbl, "DEL");
  lv_obj_center(delLbl);
  lv_obj_add_event_cb(delBtn, special_event_cb, LV_EVENT_CLICKED, (void*)"DEL");

  lv_obj_t *numBtn = lv_btn_create(keyboardContainer);
  lv_obj_set_size(numBtn, 64, 44);
  lv_obj_align(numBtn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
  lv_obj_t *numLbl = lv_label_create(numBtn);
  lv_label_set_text(numLbl, "123");
  lv_obj_center(numLbl);
  lv_obj_add_event_cb(numBtn, special_event_cb, LV_EVENT_CLICKED, (void*)"123");

  lv_obj_t *symBtn = lv_btn_create(keyboardContainer);
  lv_obj_set_size(symBtn, 64, 44);
  lv_obj_align(symBtn, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
  lv_obj_t *symLbl = lv_label_create(symBtn);
  lv_label_set_text(symLbl, "#+=");
  lv_obj_center(symLbl);
  lv_obj_add_event_cb(symBtn, special_event_cb, LV_EVENT_CLICKED, (void*) "#+=");

  keyboardCreated = true;

}


// --- BLE callbacks (do not touch LVGL directly) ---
void onBleConnect() {
  Serial.println("BLE connected");
  bleConnected = true;
}

void onBleDisconnect() {
  Serial.println("BLE disconnected");
  bleConnected = false;
  keyboardNeedsUpdate = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  Serial.println("Starting...");

  // I2C for touch
  Wire.begin(IIC_SDA, IIC_SCL);
  IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
  FT3168.reset(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS));

  // display init
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(120);

  // LVGL init
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_WIDTH * LCD_HEIGHT / 20);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // input device (touch)
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // lv_tick timer
  const esp_timer_create_args_t periodic_timer_args = { .callback = &lv_tick_task, .name = "lv_tick" };
  esp_timer_handle_t periodic_timer;
  esp_timer_create(&periodic_timer_args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, LVGL_TICK_PERIOD_MS * 1000);

  // initial status label
  statusLabel = lv_label_create(lv_scr_act());
  lv_label_set_text(statusLabel, "Attivazione Bluetooth...");
  lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 0);

  // BLE init
  bleKeyboard.onConnect(onBleConnect);
  bleKeyboard.onDisconnect(onBleDisconnect);
  bleKeyboard.begin();
}

// --- LOOP ---
void loop() {
  lv_timer_handler();
  if(digitalRead(0) == LOW) {
    returnToLauncher();
  }

  // create keyboard in LVGL context when BLE connected
  if (bleConnected && !keyboardCreated) {
    if (statusLabel) { lv_obj_del(statusLabel); statusLabel = nullptr; }
    createKeyboard();
  }

  // clear keyboard on disconnect
  if (!bleConnected && keyboardCreated) {
    clearKeyboard();
    keyboardCreated = false;
    if (!statusLabel) {
      statusLabel = lv_label_create(lv_scr_act());
      lv_label_set_text(statusLabel, "In attesa di connessione...");
      lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 0);
    }
  }

  if (keyboardNeedsUpdate) {
    keyboardNeedsUpdate = false;
    if (keyboardCreated) createKeyboard();
  }

  delay(5);
}

void returnToLauncher() {
    // Ottieni la partizione di boot attiva (launcher)
    const esp_partition_t* launcherPartition = esp_ota_get_next_update_partition(nullptr);

    if (!launcherPartition) {
        Serial.println("Errore: impossibile trovare la partizione del launcher!");
        return;
    }

    // Imposta la partizione del launcher come bootable
    if (esp_ota_set_boot_partition(launcherPartition) != ESP_OK) {
        Serial.println("Errore: impossibile cambiare partizione di boot");
        return;
    }

    Serial.println("Riavvio per tornare al launcher...");
    delay(500);
    esp_restart();  // Riavvia ESP32, partirà dal launcher
}
