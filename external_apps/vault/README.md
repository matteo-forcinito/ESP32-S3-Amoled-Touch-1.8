# Vault

App esterna per password e credenziali: archivio cifrato su microSD, scrittura
sul PC come tastiera (Bluetooth o USB) e dashboard HTTPS locale abbinata dal
dispositivo. Riutilizza il codice condiviso `watch-os-common/vault/main`.

- Build: ambiente ESP-IDF 6.1, `idf.py build`.
- Versione predefinita: 0.1.0-dev.1; override `VAULT_VERSION`.
- Build QA (`VAULT_QA=1`): solo dati sintetici nella cartella `vault-qa` della SD.
- Console di sviluppo e core dump disattivati; allarmi e gestione energetica di base attivi.
- Cifratura: PBKDF2-HMAC-SHA256 600.000 iterazioni (acceleratore SHA), AES-256-GCM,
  due snapshot cifrate. Nessuna password principale salvata sul dispositivo.
- La cifratura protegge la SD a riposo, non da firmware malevolo o PC compromesso.

Icona: `python watch-os-common/vault/tools/create_vault_icon.py icon.png`.
