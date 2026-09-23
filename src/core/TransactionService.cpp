#include "core/TransactionService.h"

#include "core/Tag.h"
#include "db/AccountRepository.h"
#include "db/CategoryRepository.h"
#include "db/TagRepository.h"
#include "db/TransactionRepository.h"

#include <QCoreApplication>
#include <QSqlDatabase>

namespace {

TransactionService::Result failure(const QString &message)
{
    return {std::nullopt, message};
}

QString tr(const char *text)
{
    return QCoreApplication::translate("TransactionService", text);
}

// Ripulisce le etichette (spazi, '#' iniziale, doppioni senza distinguere maiuscole)
// e le ordina. Messaggio d'errore, o stringa vuota se vanno bene.
QString normalizeTags(QStringList &tags)
{
    QStringList clean;
    for (const QString &raw : std::as_const(tags)) {
        const QString tag = normalizeTagName(raw);
        if (tag.isEmpty())
            continue;
        if (tag.size() > TransactionService::kMaxTagLength)
            return tr("Un'etichetta può avere al massimo %1 caratteri.").arg(TransactionService::kMaxTagLength);
        if (!clean.contains(tag, Qt::CaseInsensitive))
            clean.append(tag);
    }
    if (clean.size() > TransactionService::kMaxTags)
        return tr("Puoi usare al massimo %1 etichette per movimento.").arg(TransactionService::kMaxTags);
    clean.sort(Qt::CaseInsensitive);
    tags = clean;
    return {};
}

// Restituisce un messaggio d'errore, o una stringa vuota se il movimento è valido.
// Se valido, completa `t` (descrizione ed etichette ripulite, nome categoria).
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
    if (const QString error = normalizeTags(t.tags); !error.isEmpty())
        return error;

    t.categoryName = category->name;
    t.description = t.description.trimmed();
    return {};
}

// Collega le etichette al movimento, creando quelle nuove (senza scadenza né tetto).
bool saveTags(qint64 userId, const Transaction &t)
{
    QList<qint64> tagIds;
    for (const QString &name : t.tags) {
        const auto id = TagRepository::findOrCreate(userId, name);
        if (!id)
            return false;
        tagIds.append(*id);
    }
    return TagRepository::setForTransaction(t.id, tagIds);
}

} // namespace

TransactionService::Result TransactionService::create(qint64 userId, const Transaction &transaction)
{
    Transaction t = transaction;
    if (const QString error = validate(userId, t); !error.isEmpty())
        return failure(error);

    // Movimento ed etichette insieme: o tutto o niente.
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    const auto id = TransactionRepository::insert(t);
    if (id)
        t.id = *id;
    if (!id || !saveTags(userId, t) || !db.commit()) {
        db.rollback();
        return failure(tr("Impossibile salvare il movimento."));
    }
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

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    if (!TransactionRepository::update(t, userId) || !saveTags(userId, t) || !db.commit()) {
        db.rollback();
        return failure(tr("Impossibile salvare il movimento."));
    }
    return {t, {}};
}

bool TransactionService::remove(qint64 userId, qint64 transactionId)
{
    return TransactionRepository::remove(transactionId, userId);
}

QStringList TransactionService::tagSuggestions(qint64 userId)
{
    return TagRepository::namesForUser(userId);
}
