# Deck

App esterna: tasti programmabili per il PC (tastiera USB o Bluetooth), come uno
Stream Deck. Codice condiviso in `watch-os-common/deck/main`.

- Profili pronti: Riunioni (Teams, Meet, Zoom), Windows, Codice (VS Code),
  Browser, Media (anche YouTube), Presentazioni.
- Manopola: il knob (Watcher, Knob) o la striscia da trascinare in basso;
  ogni profilo sceglie volume, scorrimento, zoom, schede, annulla/ripeti, frecce.
- Profili personali dal PC: Menu > Configura dal PC, apri l'indirizzo mostrato e
  inserisci il codice. Ogni tasto: etichetta, simbolo, colore, azione
  (`ctrl+shift+m`, `ctrl+k ctrl+c`, `win+r; wait:300; type:notepad; enter`,
  `media:mute`, `wheel:-3`), anche registrando la combinazione dalla tastiera.
  Salvati in `config/deck.json` (microSD o flash interna).
- Build: `idf.py build`. Versione: override `DECK_VERSION`.
- Test del parser: `python watch-os-common/deck/tests/run_tests.py`.
