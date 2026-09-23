#pragma once

#include "core/Transaction.h"

#include <QList>

#include <optional>

// Come per i conti, ogni funzione richiede lo userId: si possono toccare solo
// i movimenti dei conti che appartengono a quell'utente.
namespace TransactionRepository {

// Dal più recente al più vecchio.
QList<Transaction> listForAccount(qint64 accountId, qint64 userId);

std::optional<Transaction> find(qint64 transactionId, qint64 userId);

// `transaction.id` viene ignorato. Il chiamante deve aver già verificato che il conto sia dell'utente.
std::optional<qint64> insert(const Transaction &transaction, QString *error = nullptr);

// Il conto del movimento non cambia.
bool update(const Transaction &transaction, qint64 userId, QString *error = nullptr);

bool remove(qint64 transactionId, qint64 userId);

} // namespace TransactionRepository
