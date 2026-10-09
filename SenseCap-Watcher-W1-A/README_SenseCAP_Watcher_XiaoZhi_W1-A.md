# SenseCAP Watcher for XiaoZhi --- W1-A

## Manuale tecnico per lo sviluppo firmware

> **Scopo:** raccogliere in un unico documento le informazioni pratiche
> per iniziare a sviluppare firmware personalizzato sul **Seeed Studio
> SenseCAP Watcher for XiaoZhi, modello W1-A**.
>
> **Nota sull'accuratezza:** le informazioni qui sotto distinguono le
> specifiche pubblicate per la famiglia SenseCAP Watcher dai dettagli
> che devono essere confermati sulla revisione esatta W1-A. Il nome
> commerciale è simile a quello del Watcher originale con coprocessore
> AI Himax: non assumere che firmware, pinout, partizioni o procedura di
> flashing siano intercambiabili tra le due edizioni. Per i pin non
> documentati con certezza, il README li segnala invece di inventarli.

## Indice

-   [1. Identificazione e panoramica](#1-identificazione-e-panoramica)
-   [2. Hardware](#2-hardware)
-   [3. Sensori e periferiche](#3-sensori-e-periferiche)
-   [4. Pinout e connettori](#4-pinout-e-connettori)
-   [5. Schemi elettrici e
    documentazione](#5-schemi-elettrici-e-documentazione)
-   [6. Stack software consigliato](#6-stack-software-consigliato)
-   [7. Repository e firmware di
    riferimento](#7-repository-e-firmware-di-riferimento)
-   [8. Preparare l'ambiente su
    Windows](#8-preparare-lambiente-su-windows)
-   [9. Compilare, flashare e
    monitorare](#9-compilare-flashare-e-monitorare)
-   [10. Esempi iniziali](#10-esempi-iniziali)
-   [11. Architettura consigliata per un firmware
    custom](#11-architettura-consigliata-per-un-firmware-custom)
-   [12. Espansione Grove: esempio I²C con
    DHT20](#12-espansione-grove-esempio-i²c-con-dht20)
-   [13. Partizioni, backup e
    recupero](#13-partizioni-backup-e-recupero)
-   [14. Debug e problemi comuni](#14-debug-e-problemi-comuni)
-   [15. Checklist prima del primo
    flash](#15-checklist-prima-del-primo-flash)
-   [16. Riferimenti tecnici](#16-riferimenti-tecnici)

------------------------------------------------------------------------

## 1. Identificazione e panoramica

Il **SenseCAP Watcher for XiaoZhi W1-A** è un dispositivo embedded
compatto orientato all'interazione vocale con XiaoZhi AI. La piattaforma
Watcher integra un microcontrollore ESP32-S3, display, audio,
videocamera e connettività wireless; la famiglia Watcher comprende
inoltre una versione con coprocessore di visione Himax. La presenza e
l'utilizzabilità del coprocessore **devono essere confermate per la
specifica variante XiaoZhi W1-A** prima di progettare una pipeline di
inferenza.

### Cosa si può sviluppare

-   Firmware ESP-IDF in C/C++.
-   Interfacce LVGL/display e gestione della rotella/pulsante.
-   Applicazioni vocali e integrazioni di rete.
-   Client HTTP, MQTT e integrazioni con Home Assistant o servizi
    locali.
-   Lettura di sensori esterni attraverso il connettore Grove I²C.
-   Logica locale, gestione impostazioni e aggiornamenti OTA, se
    implementati nel firmware scelto.

### Dati da non dare per scontati

-   **Non** usare alla cieca le immagini firmware della versione AI
    Vision/Watcher originale.
-   **Non** copiare gli indirizzi di flashing di una guida generica
    senza confrontarli con la tabella partizioni della variante.
-   **Non** assumere che il coprocessore Himax sia presente o che sia
    accessibile nello stesso modo della versione classica.
-   **Non** collegare segnali a 5 V direttamente ai GPIO ESP32-S3: i
    GPIO lavorano a logica 3,3 V e non sono tolleranti a 5 V.
-   **Non** alimentare il dispositivo con tensioni superiori a 5 V sulla
    porta di alimentazione.

## 2. Hardware

### Specifiche della famiglia SenseCAP Watcher

  -----------------------------------------------------------------------
  Elemento                Informazione pubblicata Note per W1-A XiaoZhi
                          per la famiglia Watcher 
  ----------------------- ----------------------- -----------------------
  MCU principale          Espressif ESP32-S3,     Da verificare come
                          fino a 240 MHz          configurazione
                                                  effettiva della scheda;
                                                  usare target `esp32s3`
                                                  solo dopo conferma

  RAM esterna             La documentazione del   Quantità effettiva da
                          Watcher classico indica verificare con log di
                          8 MB PSRAM              boot/IDF

  Flash ESP32             Il Watcher classico è   Verificare il
                          documentato con 32 MB   chip/partizioni del
                                                  W1-A prima di flashare

  Coprocessore AI         Himax HX6538 / WiseEye2 Non dare per scontato
                          (Cortex-M55 +           che sia
                          Ethos-U55) sulla        presente/abilitato
                          versione classica       sulla variante XiaoZhi

  Display                 Display touch           Verificare pannello e
                          circolare, 1,45", 412 × driver sul firmware
                          412 nella               W1-A
                          documentazione del      
                          Watcher classico        

  Camera                  OV5647, campo visivo    Verificare variante,
                          dichiarato 120° sul     collegamento e supporto
                          Watcher classico        firmware

  Audio in ingresso       Microfono integrato     Tipo, codec e pin non
                                                  vanno indovinati:
                                                  consultare schema e
                                                  sorgente della board

  Audio in uscita         Altoparlante integrato, Verificare
                          1 W dichiarato per il   amplificatore e
                          Watcher classico        percorso audio della
                                                  variante

  Wireless                Wi-Fi 2,4 GHz 802.11    ESP-IDF supporta
                          b/g/n; Bluetooth LE 5   Wi-Fi/BLE su ESP32-S3
                          nella documentazione    
                          della famiglia          

  Input utente            Rotella con rotazione e Verificare eventi e
                          pressione               driver nel progetto
                                                  XiaoZhi

  Indicatore              LED RGB nella           GPIO/driver specifici
                          documentazione della    da ricavare dal
                          famiglia                progetto board

  Espansione              Connettore Grove I²C    Pin pubblicati: SCL
                                                  GPIO48, SDA GPIO47

  USB-C                   La famiglia Watcher     Verificare fisicamente
                          documenta una porta per quale porta del W1-A
                          alimentazione e una per supporta dati
                          programmazione          

  Alimentazione           5 V DC                  Non superare 5 V

  Batteria                3,7 V, 400 mAh come     Verificare presenza e
                          backup nella scheda     comportamento effettivi
                          tecnica del Watcher     nel W1-A
                          classico                
  -----------------------------------------------------------------------

**Importante:** questa tabella è una base di lavoro per la famiglia
Watcher, non una certificazione che ogni singolo componente sia identico
nella revisione XiaoZhi W1-A. La fonte hardware del Watcher classico è
inclusa nei riferimenti; usare il firmware e lo schema specifici della
variante quando disponibili.

### Processore ESP32-S3

L'ESP32-S3 è un SoC dual-core Xtensa LX7 con Wi-Fi 2,4 GHz e Bluetooth
LE. È adatto a: - gestione UI e input; - networking; - elaborazione
audio e protocolli vocali; - controllo di periferiche I²C, SPI, UART,
GPIO e PWM; - coordinamento di eventuali acceleratori esterni.

Le capacità esatte disponibili dipendono dal modulo montato, dalla
configurazione della memoria, dalla tabella partizioni e dal framework
scelto.

### Coprocessore AI e camera

La documentazione del Watcher classico menziona il processore visivo
Himax HX6538-A (WiseEye2), che integra Arm Cortex-M55 ed Ethos-U55. È un
percorso distinto dall'ESP32-S3: quando presente, non va trattato come
un normale sensore I²C. Firmware, modelli e tool di flashing possono
essere separati.

Prima di pianificare inferenza locale: 1. Identificare il codice
prodotto/revisione stampato sul dispositivo. 2. Consultare lo schema
della variante. 3. Verificare nel firmware di fabbrica se esiste un
secondo dispositivo USB/seriale. 4. Verificare quali API e modelli sono
supportati dalla versione XiaoZhi.

## 3. Sensori e periferiche

  -----------------------------------------------------------------------
  Componente              Ruolo                   Interfaccia / note
  ----------------------- ----------------------- -----------------------
  Camera (OV5647 nella    Acquisizione immagini   Interfaccia e pin
  famiglia classica)                              specifici da leggere
                                                  nello schema; non
                                                  assumere che sia
                                                  accessibile
                                                  direttamente dal codice
                                                  applicativo ESP32

  Microfono integrato     Acquisizione voce       Di solito percorso
                                                  audio digitale tramite
                                                  codec/driver; usare la
                                                  configurazione della
                                                  board nel firmware di
                                                  riferimento

  Altoparlante integrato  Risposta vocale e suoni Uscita tramite
                                                  amplificatore audio;
                                                  non collegare
                                                  direttamente a un GPIO

  Touchscreen             UI touch                Driver pannello e
                                                  controller touch
                                                  dipendono dalla
                                                  revisione

  Rotella/pulsante        Navigazione e           Gestione tramite driver
                          push-to-talk            della board; verificare
                                                  debounce e modalità
                                                  pressione lunga

  LED RGB                 Stato/feedback          GPIO e polarità da
                                                  confermare nel progetto
                                                  board

  Grove I²C               Sensori esterni         SCL GPIO48, SDA GPIO47;
                                                  alimentazione 3,3 V

  Wi-Fi/BLE               Rete e provisioning     Supportati dal SoC
                                                  ESP32-S3
  -----------------------------------------------------------------------

La camera, il microfono e lo schermo sono periferiche integrate, non
necessariamente "sensori" autonomi con API semplici. La strada più
affidabile è riutilizzare i driver della board e i componenti del
firmware XiaoZhi compatibile, anziché ricostruire il routing elettrico
da zero.

## 4. Pinout e connettori

### Connettore Grove (J5)

Il pinout documentato per il connettore Grove del Watcher è:

  Pin Grove   Segnale   ESP32-S3 / alimentazione
  ----------- --------- --------------------------
  1           SCL       GPIO48 / I²C SCL
  2           SDA       GPIO47 / I²C SDA
  3           VCC       3,3 V (`GROVE_3.3V`)
  4           GND       GND

Per il bus: - Protocollo: I²C. - La frequenza va concordata con tutti i
dispositivi collegati; 100 kHz è una partenza prudente, 400 kHz è
possibile se i moduli lo supportano e il cablaggio è adeguato. -
Verificare la presenza di resistenze di pull-up sulla scheda e sul
modulo sensore prima di aggiungerne altre. - Il livello logico è 3,3 V.

### Header di espansione aggiuntivo

La documentazione del Watcher classico riporta un header femmina 2 × 4
con I²C, due GPIO, due GND, un'uscita 3,3 V e un ingresso 5 V. **Non è
prudente assegnare numeri GPIO all'header senza consultare lo schema
esatto W1-A**: la disposizione fisica e le connessioni devono essere
verificate sulla revisione in possesso.

### GPIO non documentati

Non esiste in questo documento una mappa completa e garantita di tutti i
GPIO interni (display, touch, camera, audio, LED, encoder, SD e
alimentazione). Inventare una tabella pin-to-peripheral sarebbe
rischioso: alcuni GPIO possono essere condivisi, riservati al boot o
connessi a circuiti di alimentazione. La tabella completa deve provenire
dallo schema della revisione esatta e dal file di configurazione della
board nel firmware.

## 5. Schemi elettrici e documentazione

### Documenti da conservare nel repository del firmware

Crea una cartella `docs/hardware/` e archivia, se la licenza e i termini
di distribuzione lo consentono:

-   schema elettrico della revisione W1-A effettiva;
-   datasheet ESP32-S3 e del modulo montato;
-   datasheet del display e del controller touch;
-   documentazione del codec/amplificatore audio, se presente;
-   schema e datasheet del modulo camera;
-   pinout dei connettori Grove e header;
-   tabella partizioni del firmware originale;
-   versione e hash dei binari di fabbrica.

### Schema pubblico della famiglia Watcher

Il repository open hardware Seeed contiene lo schema del **SenseCAP
Watcher v1.0**, datasheet e firmware di fabbrica. È un riferimento
importante, ma va confrontato con il W1-A XiaoZhi: non assumere che ogni
componente o revisione sia identico.

-   Repository hardware:
    https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher
-   Cartella Hardware:
    https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher/tree/main/Hardware
-   Documentazione hardware Watcher:
    https://wiki.seeedstudio.com/watcher_hardware_overview/
-   Prodotto W1-A:
    https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html

**Suggerimento pratico:** scarica il PDF dello schema e cerca i nomi dei
net (`GPIO47`, `GPIO48`, `I2C`, `LCD`, `MIC`, `SPK`, `CAM`, `SD`). Per
ogni periferica, annota connettore, GPIO, alimentazione, reset,
interrupt e indirizzo I²C se applicabile.

## 6. Stack software consigliato

### ESP-IDF

Il percorso raccomandato per firmware nativo è **ESP-IDF**, il framework
ufficiale Espressif: - toolchain C/C++; - FreeRTOS; - driver
GPIO/I²C/SPI/UART; - Wi-Fi e Bluetooth LE; - NVS per configurazioni
persistenti; - OTA e partizioni; - logging, tracing e diagnostica.

Il repository SDK del Watcher indica ESP-IDF e documenta una toolchain
storicamente basata su ESP-IDF 5.1/5.2.1. Il firmware XiaoZhi può avere
requisiti più specifici: seguire la versione dichiarata dal branch
XiaoZhi/Seeed selezionato, evitando di aggiornare IDF arbitrariamente.

### Repository di riferimento

  ----------------------------------------------------------------------------------------------------------------
  Repository / pagina                 Utilità
  ----------------------------------- ----------------------------------------------------------------------------
  XiaoZhi ESP32                       Firmware vocale di base e configurazione board:
                                      https://github.com/78/xiaozhi-esp32

  SDK firmware SenseCAP Watcher       Driver, componenti ed esempi Watcher:
                                      https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware

  Open hardware SenseCAP Watcher      Schema, firmware di fabbrica e documenti:
                                      https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher

  Guida Seeed XiaoZhi AI              Configurazione del Watcher con XiaoZhi:
                                      https://wiki.seeedstudio.com/sensecap_watcher_for_xiaozhi_ai/

  Guida Grove/MCP                     Esempio sensore I²C esterno:
                                      https://wiki.seeedstudio.com/extending_grove_with_mcp/

  ESP-IDF Get Started                 Installazione toolchain:
                                      https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/
  ----------------------------------------------------------------------------------------------------------------

### Versione ESP-IDF

Non fissare la versione solo in base a questo README. Prima di creare il
progetto: 1. Aprire il README e i file di dipendenze del repository
XiaoZhi scelto. 2. Leggere la versione IDF richiesta dal progetto. 3.
Installare quella versione dell'ESP-IDF e il relativo toolchain. 4.
Conservare `sdkconfig`, `sdkconfig.defaults`, `dependencies.lock` e la
versione IDF nel repository.

## 7. Repository e firmware di riferimento

### XiaoZhi per SenseCAP Watcher

La guida Seeed per "Visual Trigger and AI Flash" indica questo flusso
generale: 1. Clonare il repository XiaoZhi ESP32. 2. Selezionare il
target `esp32s3`. 3. Aprire `idf.py menuconfig`. 4. Selezionare il
profilo board **SenseCAP Watcher** quando presente nel branch
utilizzato. 5. Configurare lingua, wake word, AEC e opzioni applicabili.
6. Compilare e flashare secondo la guida della specifica variante.

Riferimento: https://wiki.seeedstudio.com/visual_trigger_and_ai_flash/

**Attenzione:** il profilo "SenseCAP Watcher" potrebbe riferirsi al
Watcher con funzioni di visione, non necessariamente alla variante
XiaoZhi-only. Se il menu o i sorgenti non identificano esplicitamente
W1-A/XiaoZhi, non forzare la configurazione: controlla i file `boards/`,
`Kconfig`, `sdkconfig.defaults` e la guida associata al firmware
installato.

### SDK Watcher

Il repository `Seeed-Studio/SenseCAP-Watcher-Firmware` fornisce
componenti ed esempi basati su ESP-IDF. Le istruzioni generiche
pubblicate sono:

``` bash
git clone https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware.git
cd SenseCAP-Watcher-Firmware
git submodule update --init
```

Esaminare prima `README.md`, `examples/`, `components/` e `.gitmodules`.
I nomi e la disponibilità degli esempi possono cambiare nel tempo. Il
repository SDK non va confuso con il firmware XiaoZhi completo.

## 8. Preparare l'ambiente su Windows

Procedura generale per un PC Windows con VS Code:

1.  Installare **Visual Studio Code**.

2.  Installare l'estensione **Espressif IDF**.

3.  Installare la versione ESP-IDF richiesta dal repository target
    tramite l'installer/gestione dell'estensione.

4.  Aprire il terminale ESP-IDF fornito dall'estensione.

5.  Installare Git.

6.  Clonare il repository scelto.

7.  Collegare il dispositivo con un cavo USB-C che supporti i dati.

8.  Aprire Gestione dispositivi e identificare la porta COM.

9.  Verificare la toolchain:

    ``` bash
    idf.py --version
    ```

10. Verificare il target del progetto:

``` bash
idf.py set-target esp32s3
```

Il comando `set-target` è appropriato soltanto se il progetto usa
ESP32-S3. Se il repository contiene una configurazione specifica della
board, seguire quella configurazione prima di sovrascriverla.

### Identificare la porta seriale

Il dispositivo può esporre una porta seriale USB/JTAG o un bridge
USB-UART, a seconda del circuito e del firmware. Non dare per scontato
che ogni porta COM visibile sia quella dell'ESP32. Se esistono più
porte, annotarle e provarle prima con un'operazione di sola
lettura/monitoraggio.

## 9. Compilare, flashare e monitorare

### Compilazione

Dalla directory di un progetto ESP-IDF:

``` bash
idf.py build
```

### Flash e monitor

Dopo aver identificato la porta corretta:

``` bash
idf.py -p COM7 flash monitor
```

Sostituire `COM7` con la propria porta. Per uscire dal monitor seriale,
normalmente usare `Ctrl+]`.

### Prima di flashare

-   Salvare la versione del firmware attualmente installato.
-   Fare un backup delle partizioni che contengono configurazione,
    calibrazione, identità o credenziali, se possibile.
-   Verificare la tabella partizioni del progetto.
-   Assicurarsi che il firmware sia per **W1-A XiaoZhi** e non per un
    altro Watcher.
-   Non usare un comando `write_flash` copiato da una guida del Watcher
    classico senza aver confrontato indirizzi e dimensioni.
-   Preferire la procedura di flashing documentata dal repository
    specifico della variante.

## 10. Esempi iniziali

Questi esempi sono frammenti didattici per testare il framework ESP-IDF.
Non sono un firmware completo e non controllano le periferiche integrate
del Watcher senza i driver corretti.

### 10.1 Log seriale minimo

``` c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "custom_app";

void app_main(void)
{
    ESP_LOGI(TAG, "Avvio firmware custom");
    while (1) {
        ESP_LOGI(TAG, "Sistema attivo");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 10.2 GPIO: usare solo un pin confermato libero

Non usare GPIO47/48 per questo test: sono assegnati al bus Grove I²C.
Non scegliere un GPIO interno a caso. Dopo aver identificato nello
schema un GPIO libero e accessibile, impostare `TEST_GPIO` con il numero
corretto:

``` c
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TEST_GPIO GPIO_NUM_N   // Sostituire N con un GPIO verificato libero
static const char *TAG = "gpio_test";

void app_main(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << TEST_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));

    int level = 0;
    while (1) {
        level = !level;
        ESP_ERROR_CHECK(gpio_set_level(TEST_GPIO, level));
        ESP_LOGI(TAG, "GPIO level=%d", level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

`GPIO_NUM_N` è un segnaposto intenzionale: il codice non compila finché
non viene sostituito con una costante GPIO valida e verificata sulla
scheda. Collegare un LED solo con resistenza limitatrice e rispettando i
limiti elettrici.

### 10.3 Rilevare dispositivi sul bus I²C

ESP-IDF ha cambiato alcune API I²C tra versioni. Usare l'esempio
`i2c_tools` o quello fornito con la versione installata, configurando
SDA=47 e SCL=48 solo dopo aver confermato il pinout della variante. Uno
scanner I²C deve limitarsi a inviare probe agli indirizzi; non scrivere
registri su dispositivi sconosciuti.

### 10.4 Persistenza con NVS

Per salvare impostazioni (ad esempio modalità, endpoint o preferenze),
usare NVS anziché scrivere indirizzi flash arbitrari. Includere
`nvs_flash.h`, inizializzare NVS e gestire il caso in cui sia necessario
cancellare/ricreare il namespace. **Non cancellare tutta la flash per
risolvere un errore NVS** senza un backup e senza sapere quali dati
verrebbero persi.

## 11. Architettura consigliata per un firmware custom

Una struttura modulare rende più facile mantenere il progetto:

``` text
main/
  app_main.c
  system/
    system.c
    system.h
  board/
    board_config.h       # pin verificati, revisione hardware
    board_init.c
  drivers/
    display/
    touch/
    audio/
    camera/
    encoder/
    status_led/
    grove_i2c/
  services/
    network/
    settings/
    diagnostics/
  apps/
    home/
    sensor_monitor/
    voice/
  CMakeLists.txt
components/
docs/
  hardware/
  firmware/
```

### Principi

-   **Board layer:** unica fonte di verità per pin, polarità, reset e
    periferiche.
-   **Driver:** inizializza e controlla un componente hardware; non
    contiene logica applicativa.
-   **Services:** rete, impostazioni, sensori, telemetria e
    aggiornamenti.
-   **Apps:** schermate e funzioni utente.
-   **Eventi:** rotella, pressione breve/lunga, touch, rete e risultati
    sensori vengono convertiti in eventi applicativi.
-   **Configurazione:** mantenere separati `sdkconfig.defaults` e
    impostazioni personali.
-   **Logging:** usare tag ESP-IDF e livelli di log; evitare stampe
    frequenti nel percorso audio real-time.

### Ordine d'inizializzazione suggerito

1.  Logging e diagnostica.
2.  NVS e configurazione.
3.  GPIO sicuri e alimentazioni controllabili.
4.  I²C e sensori esterni.
5.  Display e input.
6.  Audio e camera, riutilizzando i driver della board.
7.  Wi-Fi/BLE.
8.  Applicazione e task FreeRTOS.

L'ordine esatto deve rispettare le dipendenze e il firmware base scelto.
Non inizializzare due volte lo stesso controller se il framework XiaoZhi
lo ha già configurato.

## 12. Espansione Grove: esempio I²C con DHT20

La guida Seeed per l'espansione Grove documenta il sensore di
temperatura/umidità DHT20:

  Proprietà               Valore
  ----------------------- ------------------------------------------------------
  Bus                     I²C
  Indirizzo               `0x38`
  SDA                     GPIO47
  SCL                     GPIO48
  Alimentazione sensore   usare 3,3 V sul connettore Grove
  Frequenza               fino a 400 kHz secondo la documentazione del sensore

Procedura: 1. Spegnere il Watcher prima di collegare il modulo. 2.
Collegare il sensore al Grove. 3. Riaccendere il dispositivo. 4. Usare
uno scanner I²C compatibile con la versione ESP-IDF installata. 5.
Verificare che risponda all'indirizzo `0x38`. 6. Integrare il driver
DHT20 e controllare il datasheet per sequenza di misura, attesa e
conversione. 7. Gestire errori I²C e sensore non presente senza bloccare
l'interfaccia o il task audio.

Non collegare contemporaneamente altri dispositivi con indirizzo `0x38`
sullo stesso bus senza un multiplexer o una soluzione di indirizzamento.

## 13. Partizioni, backup e recupero

Il layout flash non è una proprietà universale del nome "Watcher":
dipende dal firmware e dalla revisione. Il Watcher classico pubblica una
procedura di flashing multi-partizione e avverte esplicitamente di non
sovrascrivere la partizione che contiene le informazioni univoche del
dispositivo (EUI/credenziali).

Prima di cambiare firmware: - salvare `partition-table.bin` o estrarre
la tabella partizioni dal dispositivo; - eseguire un backup della flash,
se le dimensioni e gli strumenti lo consentono; - conservare
separatamente le partizioni di identità/calibrazione; - annotare MAC
address, identificativi e versione originale, senza pubblicare
credenziali; - non cancellare tutta la flash se non si conosce il metodo
per ripristinare i dati specifici della variante.

I comandi pubblicati per il Watcher classico (ad esempio indirizzi
`0x0`, `0x8000`, `0x110000` e successivi) sono **esempi specifici di
quel firmware**, non indirizzi garantiti per W1-A XiaoZhi. Non copiarli
senza verifica.

Se il dispositivo non si avvia: 1. Provare un cavo USB dati e una porta
USB affidabile. 2. Verificare se compare una porta seriale. 3.
Consultare la procedura di recovery ufficiale della variante. 4. Evitare
tentativi ripetuti di flashing con immagini per modelli diversi. 5. Se
sono state perse partizioni di autenticazione, contattare il supporto
Seeed per la procedura applicabile al proprio SKU.

## 14. Debug e problemi comuni

  -----------------------------------------------------------------------
  Sintomo                             Verifiche
  ----------------------------------- -----------------------------------
  `idf.py` non trovato                Usare il terminale ESP-IDF e
                                      controllare
                                      l'installazione/toolchain

  Nessuna porta COM                   Cavo USB dati, driver USB, porta di
                                      programmazione corretta,
                                      pressione/reset secondo la guida
                                      della variante

  Flash fallisce                      Porta corretta, modalità boot,
                                      alimentazione, velocità baud;
                                      verificare target e binari

  Firmware compila ma display nero    Board config, driver pannello,
                                      alimentazione/reset/backlight,
                                      versione SDK

  Audio assente o distorto            Configurazione codec, frequenza
                                      sample, I²S, amplificatore e
                                      alimentazione; non inventare pin

  Sensore Grove non rilevato          SDA/SCL corretti, alimentazione 3,3
                                      V, indirizzo, pull-up e velocità
                                      I²C

  Wi-Fi non si connette               Banda 2,4 GHz, credenziali, RSSI,
                                      configurazione provisioning e
                                      certificati/servizi

  Crash o reboot                      Leggere backtrace e reset reason,
                                      controllare heap/PSRAM e stack task

  Mancano funzioni di visione         Verificare che il modello sia la
                                      variante con coprocessore e che
                                      firmware/modello siano compatibili

  Il dispositivo perde associazione   Verificare che non siano state
  al servizio cloud                   cancellate partizioni d'identità o
                                      credenziali
  -----------------------------------------------------------------------

Per ogni test, salvare log seriali, versione ESP-IDF, commit Git,
configurazione board e revisione hardware.

## 15. Checklist prima del primo flash

-   [ ] Confermato SKU completo e variante **W1-A XiaoZhi**.
-   [ ] Fotografata l'etichetta del dispositivo e annotata la revisione.
-   [ ] Confermato quale USB-C supporta programmazione/dati.
-   [ ] Installata la versione ESP-IDF richiesta dal repository scelto.
-   [ ] Verificato che il progetto contenga il profilo board corretto.
-   [ ] Salvati firmware originale e tabella partizioni, dove possibile.
-   [ ] Salvate informazioni univoche/di autenticazione senza
    condividerle pubblicamente.
-   [ ] Verificati pin GPIO nello schema della revisione esatta.
-   [ ] Tenuti GPIO47/48 per il Grove I²C.
-   [ ] Nessun segnale a 5 V collegato direttamente ai GPIO.
-   [ ] Prima compilazione eseguita senza flash.
-   [ ] Procedura di recovery disponibile prima di cancellare o
    sovrascrivere la flash.

## 16. Riferimenti tecnici

Questi link sono riferimenti primari da usare come archivio documentale
del progetto:

1.  **Seeed --- SenseCAP Watcher for XiaoZhi AI**\
    https://wiki.seeedstudio.com/sensecap_watcher_for_xiaozhi_ai/
2.  **Seeed --- Hardware Overview**\
    https://wiki.seeedstudio.com/watcher_hardware_overview/
3.  **Seeed --- Grove I²C / DHT20**\
    https://wiki.seeedstudio.com/extending_grove_with_mcp/
4.  **Seeed --- Visual Trigger and AI Flash**\
    https://wiki.seeedstudio.com/visual_trigger_and_ai_flash/
5.  **Seeed --- SDK firmware**\
    https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware
6.  **Seeed --- Open hardware, schematic and factory firmware**\
    https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher
7.  **XiaoZhi ESP32**\
    https://github.com/78/xiaozhi-esp32
8.  **ESP-IDF per ESP32-S3**\
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/

------------------------------------------------------------------------

## Stato delle informazioni

Questo README fornisce una base di sviluppo concreta, ma non sostituisce
i file elettrici della revisione specifica. Le informazioni da
verificare prima di un firmware che acceda direttamente all'hardware
sono: **GPIO interni di display/touch/audio/camera/rotella/LED,
revisione dello schema, quantità di RAM/flash, layout partizioni e
presenza effettiva del coprocessore Himax nel W1-A XiaoZhi**. Non sono
state inventate perché un errore in questi dati può causare periferiche
non funzionanti o perdita delle informazioni di identità del
dispositivo.
