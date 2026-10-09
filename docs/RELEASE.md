# Aggiornamenti online e release

## Come l'orologio si aggiorna

1. **Impostazioni > Aggiornamento > Controlla aggiornamenti** scarica il *catalogo* del prodotto
   (scritto dalla CI con `tools/update_catalog.py`):

   ```json
   {
     "schema": 2,
     "version": "1.2.0", "url": "...",                  // ultima stabile (firmware vecchi)
     "channels": { "stable": {"version": "1.2.0", "url": "..."},
                   "dev":    {"version": "1.3.0-dev.4", "url": "..."} },
     "releases": [ {"version", "channel", "date", "notes", "url", "size", "sha256"}, ... ]
   }
   ```

   Offre l'ultima versione del **canale** scelto (Impostazioni > Aggiornamento > Canale):
   *Stabile*, oppure *Dev* (versioni di prova; riceve anche una stabile più nuova).

   L'indirizzo è quello impostato dalla pagina web dell'orologio oppure, se vuoto, quello
   compilato nel firmware (`CONFIG_WATCH_UPDATE_MANIFEST_URL` in `sdkconfig.defaults`):
   `https://raw.githubusercontent.com/matteo-forcinito/watch-firmware-releases/main/amoled.json`

2. Se la versione è **più alta** di quella in esecuzione, con "Installa" scarica il `.bin`
   (HTTPS, segue i redirect di GitHub), controlla che sia per lo stesso chip e lo stesso
   progetto (un firmware AMOLED viene rifiutato), lo scrive nello slot libero e verifica lo SHA-256.
3. Si riavvia nella nuova versione. Se crasha prima di finire l'avvio, il bootloader torna da
   solo alla versione precedente.

## Tutte le versioni, anche vecchie

**Impostazioni > Aggiornamento > Tutte le versioni** elenca le release del catalogo (ultime 50,
arancioni quelle dev, "in uso" quella installata) e i firmware salvati sulla scheda SD. Tocca una
versione per installarla, anche se più vecchia (con un avviso). Le impostazioni restano: i campi
aggiunti dopo quella versione vengono ignorati e tornano quando si rimette la nuova.

## Copie sulla scheda SD

Se c'è una microSD, a ogni aggiornamento (online, dal PC o da SD):

- il firmware in uso viene salvato in `/sdcard/firmware/<progetto>-<versione>.bin` (una volta
  per versione), prima di essere sostituito;
- anche quello nuovo viene salvato lì, dopo la verifica.

Così si può tornare indietro anche senza rete. Senza scheda tutto funziona dal cloud. Durante la
scrittura l'orologio mostra una schermata ferma (niente animazioni: la scrittura in flash blocca
la cache del chip più volte al secondo); l'avanzamento animato è nel browser.

## Repository

| Repo | Visibilità | Contenuto |
|---|---|---|
| `watch-os-common` | privato | sistema operativo condiviso (submodule dei firmware) |
| `SenseCap-Watcher-W1-A` | privato | firmware Watcher |
| `ESP32-S3-Amoled-Touch-1.8` | privato | firmware AMOLED |
| `esp32-s3-epaper-1.54` | privato | sveglia e-paper (solo CI: release e catalogo, flash via USB) |
| `watch-firmware-releases` | **pubblico** | solo binari: release + cataloghi `amoled.json`, `amoled.json`, `epaper.json` |

Il codice resta privato; l'orologio non contiene token. I binari pubblici non contengono
segreti (Wi-Fi, password e impostazioni stanno in NVS, non nel firmware).

## Preparazione (una volta)

1. Su GitHub crea i repo `watch-os-common` e `SenseCap-Watcher-W1-A` (privati, vuoti) e
   `watch-firmware-releases` (pubblico, **con** un README, così esiste il branch `main`).
2. Crea due token *fine-grained* (Settings > Developer settings > Personal access tokens):
   - `REPOS_TOKEN`: accesso solo a `watch-os-common`, permesso *Contents: Read-only*.
   - `RELEASES_TOKEN`: accesso solo a `watch-firmware-releases`, *Contents: Read and write*.
3. In **entrambi** i repo dei firmware: Settings > Secrets and variables > Actions >
   *New repository secret*, con quei due nomi.
4. La libreria diventa un submodule in ogni firmware (cartella `watch-os-common/`), con URL
   `https://github.com/matteo-forcinito/watch-os-common.git`. Il CMake usa il submodule se
   c'è, altrimenti la cartella affiancata `../watch-os-common`.

I token scadono: quando succede, rigenerali e aggiorna i secret (l'orologio non ne risente).

## Fare una release

```powershell
git checkout -b release/1.2.0           # canale stabile
git push -u origin release/1.2.0

git checkout -b release/1.3.0-dev.1     # canale dev (prova)
git push -u origin release/1.3.0-dev.1
```

Ordine delle versioni: `1.2.0 < 1.3.0-dev.1 < 1.3.0-dev.2 < 1.3.0`. Le release dev sono marcate
"pre-release" su GitHub.

La GitHub Action (`.github/workflows/release.yml`):

1. prende la versione dal nome del branch (`release/X.Y.Z` o `release/X.Y.Z-dev.N`, altrimenti si ferma) e compila
   con ESP-IDF 6.1, verificando che la versione nell'immagine sia quella;
2. pubblica la release `amoled-vX.Y.Z` nel repo pubblico con `amoled_watch.bin`, lo zip per il
   flash via USB e `SHA256SUMS.txt`;
3. la aggiunge al catalogo `amoled.json` nel repo pubblico: da quel momento gli orologi la vedono;
4. mette il tag `amoled-vX.Y.Z` sul commit di questo repo.

Un nuovo push sullo stesso branch ripubblica la stessa versione (utile per correggere prima che
qualcuno aggiorni). Gli orologi installano solo versioni **più alte**: per un'altra release usa
un nuovo branch (`release/1.2.1`).

Il submodule va aggiornato prima della release, se nella libreria ci sono modifiche:

```powershell
git -C watch-os-common pull origin main
git add watch-os-common
git commit -m "watch-os-common aggiornato"
```

## Flash via USB di una release

Nello zip `amoled-usb-flash-X.Y.Z.zip`:

```powershell
python -m esptool --chip esp32s3 -p COM3 -b 921600 write-flash "@flash_args"
```
