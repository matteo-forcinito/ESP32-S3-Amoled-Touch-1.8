# AMOLED Watch OS

Firmware ESP-IDF in stile smartwatch per la **Waveshare ESP32-S3-Touch-AMOLED-1.8**,
portato dal launcher Arduino (`Launcher/`, `External APPS/`) e pensato per essere
riusato sulle altre board Waveshare simili.

- Quadrante digitale + always-on, centro di controllo, notifiche, elenco app
- Web radio HTTPS/HLS (m2o, Radio Zeta, Radio 105 HipHop, Radio Italia, Jazz Radio, Virgin Radio... ) in stereo
- Sveglie (melodia o radio, con fallback garantito), timer e cronometro, meteo (Open-Meteo, senza API key)
- Telefono via Bluetooth LE con **Gadgetbridge** (notifiche, chiamate, ora, meteo, controlli musica, trova telefono)
- Pagina web di configurazione (Wi-Fi, sveglie, stazioni, impostazioni, upload app) con QR code
- App esterne da microSD (anche quelle Arduino del vecchio launcher) + SDK per crearne di nuove
- Risparmio energetico: CPU dinamica 40-240 MHz, light sleep automatico, Wi-Fi solo quando serve

## Build e flash

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
idf.py set-target esp32s3     # solo la prima volta
idf.py build
idf.py -p COM5 flash monitor
```

ESP-IDF 6.1. I componenti esterni (LVGL 9.6, driver SH8601, esp_codec_dev,
esp_audio_codec, cJSON, mDNS) vengono scaricati da `idf.py` al primo build.

> Con il cavo USB collegato a un PC il chip non entra in light sleep (la console
> resta attiva): per misurare i consumi alimentalo da batteria.

## Architettura

Ogni livello usa solo quelli sotto di lui.

```
main/                 ordine di avvio
components/
  apps/               shell (quadrante, tile) + app: radio, sveglie, meteo, timer, impostazioni...
  services/           wifi, time_sync, radio, alarm, notify, weather, ble_companion, web_server, extapp, sound
  ui/                 tema, font (Montserrat + Tiny TTF), widget (pagine, righe, slider, toast, dialoghi, tastiera, icone meteo)
  core/               lv_port (LVGL a basso consumo), power, app manager, settings (NVS), state (subject LVGL), clock
  hardware/           BSP: board.h + driver (SH8601, FT3168, AXP2101, PCF85063, ES8311, SD, pulsanti)
  extapp_sdk/         2 funzioni per le app esterne
examples/hello_app/   app esterna minima che riusa tutta la piattaforma
```

### Regole principali

- **Thread**: LVGL gira solo nel task `lvgl`. Dagli altri task si usa `ui_async(fn, arg)`
  (mai bloccante) oppure `lv_port_lock()/unlock()`.
- **Stato condiviso**: i servizi chiamano `state_set()/state_bump()` (non bloccanti, senza lock);
  la UI osserva i subject (`state_subject(STATE_BATTERY)`...). Così nessun servizio aspetta la UI.
- **Navigazione**: un'app è un `app_t` costante. `app_open("id")`, `app_back()`, `app_home()`;
  le sottopagine sono app nascoste. Indietro = swipe dal bordo sinistro o click BOOT.
- **Pulsanti**: BOOT click = indietro, BOOT lungo = home, PWR click = spegni schermo,
  PWR lungo = menu alimentazione (6 s = spegnimento hardware).

### Memoria

La RAM interna (~300 KB) serve a Wi-Fi, Bluetooth, TLS e agli stack dei task. LVGL alloca
tutto (widget, stili, glifi, layer) in PSRAM tramite `core/lv_mem_psram.c`; il log di
avvio stampa la RAM interna libera dopo ogni fase (`sys: [ui] internal ... KB free`) e la
pagina Impostazioni → Info mostra libera e minimo storico.

### Navigazione della shell

```
                 centro di controllo
                        ▲ (swipe giù)
 in riproduzione ◄── QUADRANTE ──► elenco app
                        ▼ (swipe su)
                    notifiche
```

## Consumo in idle

| Cosa | Come |
| --- | --- |
| CPU | DFS 40-240 MHz: 240 solo mentre LVGL disegna/anima o la radio decodifica (`power_cpu_boost`) |
| Sleep | light sleep automatico (tickless idle); risveglio da touch INT, BOOT, timer, Wi-Fi/BLE |
| LVGL | il task dorme finché non c'è lavoro: il timer di refresh si ferma da solo, la lettura touch parte solo su INT |
| Display | ON → DIM (-4 s) → OFF (pannello in sleep) o AOD (luminosità 24, quadrante quasi tutto nero, pixel shift) |
| Touch | modalità monitor (scansione lenta) a schermo spento, tap-to-wake |
| Wi-Fi | a riferimento contato: si accende per radio/meteo/NTP/pagina web, si spegne 8 s dopo l'ultimo uso; modem sleep |
| BLE | advertising ogni 1-1,5 s, connessione 100-200 ms con slave latency 4 |
| Audio | codec e amplificatore spenti quando non suona nulla |
| Flash | impostazioni scritte con 2 s di ritardo (gli slider non consumano la flash) |

## Web radio

Lista di default in `components/services/radio/radios_default.txt` (indirizzi HTTPS
verificati). Per modificarla: copia il file in `/sdcard/config/radios.txt` o usa la
pagina web. Formato: `nome | genere | url`. Preferiti: tieni premuta una stazione.

Ottimizzazioni HTTPS: buffer TLS in PSRAM con buffer dinamici, ripresa della sessione TLS
(session ticket) e, per l'HLS, **connessione keep-alive riusata** per segmenti e playlist:
un solo handshake invece di uno ogni 6-10 secondi. Buffer di rete 384 KB in PSRAM.

## Telefono (Bluetooth)

Servizio Nordic UART con il protocollo di Gadgetbridge per Bangle.js
(vedi `components/services/include/services/ble_companion.h`).
Su Android: installa Gadgetbridge, cerca dispositivi, aggiungi "Bangle.js AMOLED".
Il nome deve iniziare con `Bangle.js` perché Gadgetbridge lo riconosca.
Nessun pairing: chiunque vicino può connettersi finché il telefono non è collegato.

## App esterne

```
/sdcard/apps/<Nome>/<Nome>.bin     firmware (Arduino o ESP-IDF)
/sdcard/apps/<Nome>/icon.png       icona opzionale (o icon.bin in formato LVGL 9)
```

Le app della SD compaiono direttamente nel launcher (sezione **Scheda SD**, con la loro
icona; l'elenco si aggiorna ogni volta che apri il launcher). Al tocco il `.bin` viene
copiato nel secondo slot OTA con barra di avanzamento e l'orologio riavvia nell'app; se
l'app è già nello slot la copia viene saltata e si apre subito.

- Funzionano sia il `.bin` dell'app sia il `.bin` "merged" di Arduino (bootloader +
  partizioni + app): l'app viene trovata a 0x10000.
- Le app Arduino del vecchio launcher funzionano invariate (tornano con
  `esp_ota_get_next_update_partition`).
- Icone: `icon.png`, `icon.bin` LVGL 9, oppure le `icon.bin` LVGL 8 del vecchio launcher
  (convertite al volo: true color, true color alpha, chroma key, indicizzate).

Nuove app ESP-IDF: vedi `examples/hello_app` — chiama `extapp_sdk_init()` all'avvio
(qualsiasi riavvio o crash torna al launcher) e riusa board, LVGL, tema e widget.

```powershell
cd examples/hello_app
idf.py build
# copia build/hello_app.bin in /sdcard/apps/Hello/Hello.bin
```

## Aggiungere una board Waveshare

1. Copia `components/hardware/boards/waveshare_amoled_1_8.h`, cambia pin e flag `BOARD_HAS_*`.
2. Aggiungi la voce in `components/hardware/Kconfig` e in `boards/board_config.h`.
3. Se il controller del display è diverso (es. CO5300 su 1.43"/1.75"/2.06") aggiungi il
   driver in `hardware/display.c` dietro un flag della board. Pannelli rotondi: `BOARD_LCD_ROUND 1`.
4. `idf.py menuconfig` → *Board (hardware layer)*.

Core, servizi e app non cambiano.

## Aggiungere un'app interna

```c
static void create(lv_obj_t *screen, void *arg)
{
    lv_obj_t *page = ui_page(screen, "Ciao");
    ui_row(page, LV_SYMBOL_OK, UI_COLOR_GREEN, "Riga", "valore", NULL, NULL);
}

const app_t hello_app = {.id = "hello", .name = "Ciao", .icon = LV_SYMBOL_OK,
                         .color = UI_COLOR_GREEN, .create = create};
```

Dichiarala in `components/apps/apps_internal.h`, aggiungila a `s_apps[]` in `apps.c` e
il file in `components/apps/CMakeLists.txt`.

## Note

- Il codice Arduino originale è rimasto in `Launcher/` e `External APPS/` come riferimento.
- La chiave OpenWeatherMap che era nel codice Arduino (`MeteoScreen.h`) è pubblica nel
  repository: conviene revocarla. Il nuovo meteo usa Open-Meteo, che non richiede chiavi.
- Font: Montserrat (SIL Open Font License, `components/ui/fonts/OFL.txt`), ridotti ai caratteri usati.
