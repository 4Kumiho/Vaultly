#pragma once

#include <QHash>
#include <QList>
#include <QStringList>

#include <optional>

// Etichette libere dei movimenti, per utente. I nomi sono unici senza distinguere maiuscole.
namespace TagRepository {

// Nomi delle etichette dell'utente, in ordine alfabetico (per i suggerimenti).
QStringList namesForUser(qint64 userId);

// Id dell'etichetta con quel nome, creandola se non esiste.
std::optional<qint64> findOrCreate(qint64 userId, const QString &name);

// Sostituisce le etichette di un movimento.
bool setForTransaction(qint64 transactionId, const QList<qint64> &tagIds);

// Etichette dei movimenti di un conto: id movimento → nomi (in ordine alfabetico).
QHash<qint64, QStringList> namesForAccount(qint64 accountId, qint64 userId);

QStringList namesForTransaction(qint64 transactionId);

// Elimina le etichette dell'utente non più usate da nessun movimento.
void deleteUnused(qint64 userId);

} // namespace TagRepository
