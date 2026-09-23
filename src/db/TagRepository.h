#pragma once

#include "core/Tag.h"

#include <QHash>
#include <QList>
#include <QStringList>

#include <optional>

// Etichette dei movimenti, per utente. I nomi sono unici senza distinguere maiuscole.
// Ogni funzione richiede lo userId: si toccano solo le etichette di quell'utente.
namespace TagRepository {

// Nomi delle etichette dell'utente, in ordine alfabetico (per i suggerimenti).
QStringList namesForUser(qint64 userId);

// Etichette dell'utente con impostazioni e numero di movimenti che le usano.
QList<Tag> listForUser(qint64 userId);

std::optional<Tag> find(qint64 tagId, qint64 userId);

bool nameExists(qint64 userId, const QString &name, qint64 excludeTagId = 0);

std::optional<qint64> insert(qint64 userId, const Tag &tag);
bool update(const Tag &tag, qint64 userId);
// Toglie l'etichetta anche da tutti i movimenti (ON DELETE CASCADE).
bool remove(qint64 tagId, qint64 userId);

// Id dell'etichetta con quel nome, creandola (senza scadenza né tetto) se non esiste.
std::optional<qint64> findOrCreate(qint64 userId, const QString &name);

// Sostituisce le etichette di un movimento.
bool setForTransaction(qint64 transactionId, const QList<qint64> &tagIds);

// Etichette dei movimenti di un conto: id movimento → nomi (in ordine alfabetico).
QHash<qint64, QStringList> namesForAccount(qint64 accountId, qint64 userId);

QStringList namesForTransaction(qint64 transactionId);

// Uscite con l'etichetta, su tutti i conti dell'utente in `currencyCode`, con data in [from, to]
// (`from` invalido = da sempre).
qint64 spent(qint64 userId, qint64 tagId, const QString &currencyCode, const QDateTime &from, const QDateTime &to);

} // namespace TagRepository
