# AMOLED Watch OS

Firmware ESP-IDF in stile smartwatch per la **Waveshare ESP32-S3-Touch-AMOLED-1.8**,
portato dal launcher Arduino (`Launcher/`, `External APPS/`, tenuti come riferimento) e
pensato per essere riusato sulle altre board Waveshare simili.

Il firmware principale è un **sistema operativo base**, piccolo e prevedibile; le
funzioni pesanti (come la web radio HTTPS) sono **app esterne** sulla microSD.

**Sistema base**
- Quadrante digitale + always-on, centro di controllo, notifiche, launcher a griglia
- Sveglie (melodia crescente, posticipa), timer e cronometro, meteo (Open-Meteo, senza API key)
- Telefono via Bluetooth LE con **Gadgetbridge**: notifiche, chiamate, ora, meteo, controlli musica, trova telefono
- Pagina web di configurazione (Wi-Fi, sveglie, impostazioni, upload app) con QR code
- **Unità USB**: la microSD vista dal PC come chiavetta
- App esterne dalla microSD (anche quelle Arduino del vecchio launcher)
- Risparmio energetico: CPU 40-240 MHz, light sleep automatico, Wi-Fi solo quando serve

**App esterne incluse**
- `external_apps/webradio` — web radio HTTPS/HLS stereo (m2o, Radio Zeta, Radio 105 HipHop,
  Radio Italia, Jazz Radio, Virgin Radio, ...)
- `examples/hello_app` — l'app esterna minima, da copiare per crearne di nuove

## Build e flash

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
idf.py set-target esp32s3     # solo la prima volta
idf.py build
idf.py -p COM3 flash monitor
```

ESP-IDF 6.1. I componenti esterni (LVGL 9.6, driver SH8601, esp_codec_dev, TinyUSB,
cJSON, mDNS) vengono scaricati al primo build. Dopo aver cambiato `sdkconfig.defaults`
cancella `sdkconfig` (o `idf.py fullclean`) perché le nuove impostazioni vengano applicate.

> Con il cavo USB collegato a un PC il chip non entra in light sleep (la console resta
> attiva): per misurare i consumi alimentalo da batteria.

### App esterna Web Radio

```powershell
cd external_apps\webradio
idf.py set-target esp32s3
idf.py build
```

Sulla microSD:

```
/sdcard/apps/WebRadio/WebRadio.bin     <- external_apps/webradio/build/webradio.bin
/sdcard/apps/WebRadio/icon.png         <- external_apps/webradio/icon.png
/sdcard/apps/WebRadio/manifest.json    <- external_apps/webradio/manifest.json
```

(con l'app **Unità USB** o dalla pagina web). Poi Launcher → *Scheda SD* → Web Radio.
BOOT torna al sistema. Stazioni: `components/webradio/radios_default.txt`, oppure
`/sdcard/config/radios.txt` (formato `nome | genere | url`); tieni premuta una stazione
per i preferiti.

## Architettura

Ogni livello usa solo quelli sotto di lui.

```
main/                   ordine di avvio del sistema base
components/
  apps/                 shell (quadrante, tile, launcher) + app di sistema
  companion/            telefono via BLE (Gadgetbridge / protocollo Bangle.js)
  services/             wifi, time_sync, alarm, notify, weather, sound, web_server, extapp, http_stream
  webradio/             motore radio (HTTPS, HLS, MP3/AAC) - usato solo dall'app esterna
  ui/                   tema, font, widget (pagine, righe, slider, toast, dialoghi, tastiera, icone)
  core/                 lv_port, power, app manager, settings, state, clock, sys (worker, diagnostica)
  hardware/             BSP: board.h + driver (SH8601, FT3168, AXP2101, PCF85063, ES8311, SD, pulsanti)
  extapp_sdk/           2 funzioni per le app esterne
external_apps/webradio/ app esterna Web Radio
examples/hello_app/     app esterna minima
```

Le app esterne includono gli stessi componenti (`EXTRA_COMPONENT_DIRS`) ed escludono quelli
che non servono: hanno lo stesso aspetto e la stessa gestione energetica del sistema.

### Regole principali

- **Thread**: LVGL gira solo nel task `lvgl`. Dagli altri task si usa `ui_async(fn, arg)`.
- **Timer**: le callback di `esp_timer` non fanno mai lavoro vero: lo passano al worker
  di sistema con `sys_post()` (scritture NVS, Wi-Fi on/off, controllo sveglie, BLE).
- **Stato condiviso**: i servizi chiamano `state_set()/state_bump()` (non bloccanti);
  la UI osserva i subject. Nessun servizio aspetta la UI.
- **Navigazione**: un'app è un `app_t` costante (`app_open`, `app_back`, `app_home`).
  Le schermate chiuse vengono liberate solo a transizione finita, e la `destroy()` di
  un'app viene chiamata insieme alla cancellazione dei suoi widget; i comandi durante
  un'animazione vengono accodati. Vedi il commento in `core/app_manager.c`.
- **Impostazioni**: `settings_save()` + listener (es. il Bluetooth si accende/spegne da solo
  quando cambia `ble_enabled`, da qualunque schermata o dalla pagina web).
- **Pulsanti**: BOOT click = indietro, BOOT lungo = home, PWR click = spegni schermo,
  PWR lungo = menu alimentazione (6 s = spegnimento hardware).

### Navigazione della shell

```
                 centro di controllo
                        ▲ (swipe giù)
 musica telefono ◄── QUADRANTE ──► launcher (app + Scheda SD)
                        ▼ (swipe su)
                    notifiche
```

## Affidabilità e diagnostica

- **Crash**: il core dump viene salvato nella partizione `coredump`. Al riavvio il log mostra
  `sys: PREVIOUS RUN CRASHED: ... <task> @0x<pc>` e compare un avviso; Impostazioni → Info
  riporta il motivo dell'ultimo avvio. Dettagli completi: `idf.py coredump-info`.
- **Memoria**: LVGL alloca in PSRAM (`core/lv_mem_psram.c`); la RAM interna resta a Wi-Fi,
  Bluetooth e stack. Il log di avvio stampa la RAM interna libera dopo ogni fase
  (`sys: [ui] internal ... KB free`); Info mostra libera e minimo storico.
- **Task**: creati con `sys_task_create()`, che registra il motivo se non c'è memoria.
- **Light sleep**: i pin che devono mantenere lo stato (CS display, I2C, SD, amplificatore,
  INT touch, BOOT) sono esclusi dall'isolamento in `hardware/board.c`.

## Consumo in idle

| Cosa | Come |
| --- | --- |
| CPU | 240 MHz solo mentre LVGL disegna/anima (`power_cpu_boost`), altrimenti 40 MHz |
| Sleep | light sleep automatico; risveglio da touch INT, BOOT, timer, Wi-Fi/BLE |
| LVGL | il task dorme finché non c'è lavoro; la lettura touch parte solo su INT |
| Display | ON → DIM → OFF (pannello in sleep) o AOD (luminosità 24, quasi tutto nero, pixel shift) |
| Touch | modalità monitor a schermo spento, tap-to-wake |
| Wi-Fi | a riferimento contato, si spegne 8 s dopo l'ultimo uso; modem sleep |
| BLE | advertising veloce 60 s dopo avvio/disconnessione, poi ogni 1-1,5 s; connessione con slave latency |
| Audio | codec e amplificatore spenti quando non suona nulla |

## Telefono (Gadgetbridge)

1. Su Android installa **Gadgetbridge** (F-Droid o Play Store).
2. Sull'orologio: Bluetooth acceso (centro di controllo). Il nome deve iniziare con
   `Bangle.js` (predefinito: *Bangle.js AMOLED*), perché Gadgetbridge lo riconosca.
3. In Gadgetbridge: **+** → cerca → *Bangle.js AMOLED* → associa. Se chiede il pairing,
   l'orologio accetta ("just works", il legame resta salvato).
4. Nelle impostazioni del dispositivo in Gadgetbridge abilita notifiche, meteo, musica.

Nei primi 60 s dopo l'accensione (o dopo una disconnessione) l'orologio si annuncia
velocemente: è il momento migliore per cercarlo.

## App esterne

```
/sdcard/apps/<Nome>/<Nome>.bin     firmware (Arduino o ESP-IDF; anche il .bin "merged" di Arduino)
/sdcard/apps/<Nome>/icon.png       icona opzionale (o icon.bin LVGL 9, o icon.bin LVGL 8 del vecchio launcher)
/sdcard/apps/<Nome>/manifest.json  opzionale: {"name": "...", "bin": "..."}
```

Le app compaiono nel launcher (sezione *Scheda SD*). Al tocco il `.bin` viene copiato nel
secondo slot OTA (barra di avanzamento) e l'orologio riavvia nell'app; se è già nello slot
la copia viene saltata. Le app ESP-IDF chiamano `extapp_sdk_init()` all'avvio: qualsiasi
riavvio o crash torna al sistema. Le app Arduino del vecchio launcher funzionano invariate.

## Aggiungere una board Waveshare

1. Copia `components/hardware/boards/waveshare_amoled_1_8.h`, cambia pin e flag `BOARD_HAS_*`.
2. Aggiungi la voce in `components/hardware/Kconfig` e in `boards/board_config.h`.
3. Display diverso (es. CO5300 su 1.43"/1.75"/2.06"): driver in `hardware/display.c` dietro
   un flag della board. Pannelli rotondi: `BOARD_LCD_ROUND 1`.
4. `idf.py menuconfig` → *Board (hardware layer)*.

## Aggiungere un'app di sistema

```c
static void create(lv_obj_t *screen, void *arg)
{
    lv_obj_t *page = ui_page(screen, "Ciao");
    ui_row(page, LV_SYMBOL_OK, UI_COLOR_GREEN, "Riga", "valore", NULL, NULL);
}

const app_t hello_app = {.id = "hello", .name = "Ciao", .icon = LV_SYMBOL_OK,
                         .color = UI_COLOR_GREEN, .create = create};
```

Dichiarala in `components/apps/apps_internal.h`, aggiungila a `s_apps[]` in `apps.c` e il
file in `components/apps/CMakeLists.txt`.

## Note

- La chiave OpenWeatherMap che era nel codice Arduino (`MeteoScreen.h`) è pubblica nel
  repository: conviene revocarla.
- Font: Montserrat (SIL Open Font License, `components/ui/fonts/OFL.txt`), ridotti ai caratteri usati.
