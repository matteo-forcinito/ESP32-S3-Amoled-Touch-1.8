# Rilascio automatico

Esegui `./tools/release.ps1 -Version 1.8.0 -SourceRef main` con Python e la CLI GitHub ufficiale gia autenticata. Il comando avvia la build privata di produzione, attende i pacchetti, verifica checksum, identita hardware, versione e pacchetto USB, pubblica soltanto i binari nel repository watch-firmware-releases e aggiorna il catalogo OTA. Le versioni pubblicate sono immutabili.

L’orchestratore comune e watch-os-common/tools/release_firmware.py. Non legge, esporta o copia credenziali: tutte le operazioni GitHub passano dalla CLI ufficiale. Non dipende dai vecchi RELEASES_TOKEN/REPOS_TOKEN, risultati non validi. La build puo anche partire dal push di release/X.Y.Z; `--run-id` permette all’orchestratore di pubblicarne gli artefatti verificati. Il processo locale deve restare attivo fino al messaggio PUBLISHED.
