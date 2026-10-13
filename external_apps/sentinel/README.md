# Sentinel

App esterna di inventario e diagnostica IPv4 con archivio su microSD e dashboard
HTTP locale abbinata dal dispositivo. Riutilizza il codice condiviso
`watch-os-common/sentinel/main`.

- Build: ambiente ESP-IDF 6.1, `idf.py build`.
- Versione predefinita: 0.1.0; override `SENTINEL_VERSION`.
- Console di sviluppo disattivata nelle build normali; `sdkconfig.qa` solo per QA.
- Allarmi e gestione energetica di base restano attivi.
- Copertura: LAN IPv4 raggiungibile, nessuna garanzia di rilevare tutti i client.
- HTTP locale non cifra i dati; non esporre la porta su Internet.

Documentazione: https://github.com/matteo-forcinito/SenseCap-Watcher-W1-A/blob/main/docs/SENTINEL.md
