# Cielo

App esterna: gli aerei sopra di te, in tempo reale. Codice condiviso in
`watch-os-common/cielo/main`.

- Disco del cielo centrato sulla tua posizione (quella del Meteo), nord in alto:
  ogni aereo e' una freccia colorata per quota con la sua scia; tra un
  aggiornamento e l'altro (12 s) si muove con velocita' e rotta.
- "Alza lo sguardo": il volo piu' alto sul tuo orizzonte, rotta e dove guardare.
- Dettaglio volo: compagnia, rotta, aereo, quota, velocita', cupola del cielo.
- Stazione Spaziale Internazionale quando e' entro 2500 km.
- Dati: rete pubblica ADS-B api.adsb.lol, rotte api.adsbdb.com, ISS
  api.wheretheiss.at. Nessuna chiave, serve il Wi-Fi.
- Build: `idf.py build`. Versione: override `CIELO_VERSION`.
