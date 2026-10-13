# Ombra

App esterna: cosa trasmette in Bluetooth LE intorno a te. Codice condiviso in
`watch-os-common/ombra/main`.

- Riconosce tracker (AirTag e accessori Dov'e, Galaxy SmartTag, Tile, Chipolo,
  tag Google Trova il mio dispositivo), telefoni, computer, cuffie, orologi,
  beacon, casa, salute; stima la distanza dal segnale.
- **Trova**: caldo/freddo a schermo intero e contatore Geiger dall'altoparlante.
- **Scudo**: allarme se un tracker separato dal proprietario resta con te oltre la
  soglia (anche dopo il cambio di indirizzo).
- Solo ascolto: nessuna connessione, nessun dato inviato.
- Build: `idf.py build` (ESP-IDF 6.1). Versione: override `OMBRA_VERSION`.
  QA con console: `SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.qa"`.
- Test del classificatore: `python watch-os-common/ombra/tests/run_tests.py`.
