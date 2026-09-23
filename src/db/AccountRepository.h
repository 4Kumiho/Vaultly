#pragma once

#include "core/Account.h"

#include <QList>

#include <optional>

// Tutte le funzioni che toccano un conto esistente richiedono anche lo userId,
// così un utente non può mai leggere o modificare i conti di un altro.
namespace AccountRepository {

QList<Account> listForUser(qint64 userId);

std::optional<Account> find(qint64 accountId, qint64 userId);

// Confronto case-insensitive; `excludeAccountId` serve quando si rinomina un conto esistente.
bool nameExists(qint64 userId, const QString &name, qint64 excludeAccountId = 0);

std::optional<qint64> insert(qint64 userId, const QString &name, const QString &currencyCode,
                             qint64 initialBalance, QString *error = nullptr);

bool update(qint64 accountId, qint64 userId, const QString &name, qint64 initialBalance,
            QString *error = nullptr);

// Elimina il conto e, a cascata, tutti i suoi movimenti.
bool remove(qint64 accountId, qint64 userId);

int transactionCount(qint64 accountId, qint64 userId);

// Saldo iniziale + entrate − uscite.
qint64 currentBalance(qint64 accountId, qint64 userId);

} // namespace AccountRepository
