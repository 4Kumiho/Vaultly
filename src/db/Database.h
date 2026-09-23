#pragma once

#include <QString>

#include <optional>

namespace Database {

// Apre (o crea) il DB SQLite sulla connessione di default, attiva le foreign key
// e crea/aggiorna lo schema. `path` può essere ":memory:" (usato nei test).
bool open(const QString &path, QString *error = nullptr);

// Apre il DB dell'app, `dataDir/vaultly.db` (una nuova installazione parte vuota).
// Recupera anche il DB finito per errore in `dataDir/Vaultly/vaultly.db` con la 1.0.3:
// se è l'unico lo sposta, altrimenti ne importa gli utenti. Nessun dato viene cancellato
// finché non è stato copiato: in caso di conflitti il file resta come vaultly-recupero-*.db.
bool openAppDatabase(const QString &dataDir, QString *error = nullptr);

// Copia nel DB aperto gli utenti di `otherPath` (con conti, movimenti e area password)
// il cui username non esiste già. Restituisce quanti ne ha importati; `conflicts` riceve
// quanti ne ha saltati perché già presenti. Tutto o niente (una transazione).
std::optional<int> importUsersFrom(const QString &otherPath, int *conflicts = nullptr, QString *error = nullptr);

// Chiude e rimuove la connessione di default.
void close();

} // namespace Database
