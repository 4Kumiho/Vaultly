#pragma once

#include "core/Tag.h"
#include "core/Transaction.h"

#include <optional>

namespace TransactionService {

constexpr int kMaxTags = 10;
constexpr int kMaxTagLength = kMaxTagNameLength;

struct Result
{
    std::optional<Transaction> transaction; // valorizzato solo in caso di successo
    QString error;                          // messaggio da mostrare all'utente
};

// Controlla importo, categoria (deve essere del tipo giusto), data, etichette e che il conto
// sia dell'utente. Le etichette scritte che non esistono ancora vengono create.
Result create(qint64 userId, const Transaction &transaction);
Result update(qint64 userId, const Transaction &transaction);
bool remove(qint64 userId, qint64 transactionId);

// Etichette già usate dall'utente, in ordine alfabetico.
QStringList tagSuggestions(qint64 userId);

} // namespace TransactionService
