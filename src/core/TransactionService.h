#pragma once

#include "core/Transaction.h"

#include <optional>

namespace TransactionService {

struct Result
{
    std::optional<Transaction> transaction; // valorizzato solo in caso di successo
    QString error;                          // messaggio da mostrare all'utente
};

// Controlla importo, categoria (deve essere del tipo giusto), data e che il conto sia dell'utente.
Result create(qint64 userId, const Transaction &transaction);
Result update(qint64 userId, const Transaction &transaction);
bool remove(qint64 userId, qint64 transactionId);

} // namespace TransactionService
