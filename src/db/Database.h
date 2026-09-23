#pragma once

#include <QString>

namespace Database {

// Apre (o crea) il DB SQLite sulla connessione di default, attiva le foreign key
// e crea/aggiorna lo schema. `path` può essere ":memory:" (usato nei test).
bool open(const QString &path, QString *error = nullptr);

// Chiude e rimuove la connessione di default.
void close();

} // namespace Database
