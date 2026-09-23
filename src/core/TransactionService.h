#pragma once

#include "core/Transaction.h"

#include <optional>

namespace TransactionService {

constexpr int kMaxTags = 10;
constexpr int kMaxTagLength = 30;

struct Result
{
    std::optional<Transaction> transaction; // valorizzato solo in caso di successo
    QString error;                          // messaggio da mostrare all'utente
};

// Controlla importo, categoria (deve essere del tipo giusto), data, etichette e che il conto
// sia dell'utente. Le etichette nuove vengono create, quelle non più usate eliminate.
Result create(qint64 userId, const Transaction &transaction);
Result update(qint64 userId, const Transaction &transaction);
bool remove(qint64 userId, qint64 transactionId);

// Etichette già usate dall'utente, in ordine alfabetico.
QStringList tagSuggestions(qint64 userId);

} // namespace TransactionService
