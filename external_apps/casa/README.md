# Casa

App esterna: Home Assistant al polso e sulla scrivania. Codice condiviso in
`watch-os-common/casa/main`.

- Stanze di riquadri; le luci accese brillano nel loro colore reale, intense
  quanto la loro luminosità. Tocco = azione naturale (accendi/spegni, avvia la
  scena, apri/chiudi, play/pausa); pressione lunga = dettaglio con manopola
  (luminosità, posizione, velocità, volume, temperatura) da dito o knob,
  colori e temperature di bianco, modalità del clima, serratura con conferma.
- Stato in tempo reale con l'API WebSocket (`subscribe_entities`), comandi in
  coda, ping e riconnessione automatica. Funziona in locale (anche
  `homeassistant.local`, risolto via mDNS) e con Nabu Casa (wss).
- Configurazione dal PC: tocca CASA in alto > indirizzo e codice. Nella pagina:
  indirizzo di Home Assistant, token a lunga durata, preferiti scelti stanza
  per stanza (il browser li legge direttamente da Home Assistant). Il token
  resta nella NVS dell'orologio, i preferiti in `config/casa.json`.
- Build: `idf.py build`. Versione: override `CASA_VERSION`.
