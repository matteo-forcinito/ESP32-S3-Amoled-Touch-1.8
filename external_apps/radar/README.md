# Radar

App esterna: il radar della pioggia intorno a te, animato, con la previsione dei
prossimi minuti. Codice condiviso in `watch-os-common/radar/main`.

- Mappa scura (tessere OpenStreetMap scurite sull'orologio) centrata sulla
  posizione del Meteo, radar RainViewer delle ultime 2 ore (13 immagini).
- Verdetto in parole: "Pioggia tra 25 min, debole", "Sta piovendo, dovrebbe
  smettere verso le 18:15", "Pioggia a 18 km verso ovest, si avvicina: ~35 min".
  L'arrivo stimato confronta la pioggia più vicina ora e 30 minuti fa.
- Dettagli (swipe in su): pioggia prevista a 15 minuti per 2 ore e probabilità
  per 12 ore (Open-Meteo), zoom Vicino/Regione/Italia (anche con il knob).
- Build: `idf.py build`. Versione: override `RADAR_VERSION`.
- Test della parte grafica: `python watch-os-common/radar/tests/run_tests.py`.
