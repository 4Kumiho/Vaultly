#include "core/TransactionService.h"

#include "db/AccountRepository.h"
#include "db/CategoryRepository.h"
#include "db/TransactionRepository.h"

#include <QCoreApplication>

namespace {

TransactionService::Result failure(const QString &message)
{
    return {std::nullopt, message};
}

QString tr(const char *text)
{
    return QCoreApplication::translate("TransactionService", text);
}

// Restituisce un messaggio d'errore, o una stringa vuota se il movimento è valido.
// Se valido, completa `t` (descrizione ripulita, nome categoria).
QString validate(qint64 userId, Transaction &t)
{
    if (!AccountRepository::find(t.accountId, userId))
        return tr("Conto non trovato.");
    if (t.amount <= 0)
        return tr("L'importo deve essere maggiore di zero.");
    if (!t.occurredAt.isValid())
        return tr("Data non valida.");

    const auto category = CategoryRepository::find(t.categoryId);
    if (!category)
        return tr("Scegli una categoria.");
    if (category->type != t.type)
        return tr("La categoria non corrisponde al tipo di movimento.");

    t.categoryName = category->name;
    t.description = t.description.trimmed();
    return {};
}

} // namespace

TransactionService::Result TransactionService::create(qint64 userId, const Transaction &transaction)
{
    Transaction t = transaction;
    if (const QString error = validate(userId, t); !error.isEmpty())
        return failure(error);

    const auto id = TransactionRepository::insert(t);
    if (!id)
        return failure(tr("Impossibile salvare il movimento."));
    t.id = *id;
    return {t, {}};
}

TransactionService::Result TransactionService::update(qint64 userId, const Transaction &transaction)
{
    const auto existing = TransactionRepository::find(transaction.id, userId);
    if (!existing)
        return failure(tr("Movimento non trovato."));

    Transaction t = transaction;
    t.accountId = existing->accountId; // un movimento non cambia conto
    if (const QString error = validate(userId, t); !error.isEmpty())
        return failure(error);

    if (!TransactionRepository::update(t, userId))
        return failure(tr("Impossibile salvare il movimento."));
    return {t, {}};
}

bool TransactionService::remove(qint64 userId, qint64 transactionId)
{
    return TransactionRepository::remove(transactionId, userId);
}
