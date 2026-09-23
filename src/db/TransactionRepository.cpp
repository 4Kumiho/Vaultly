#include "db/TransactionRepository.h"

#include "db/TagRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

// Ora locale senza fuso: ordinabile come stringa.
const char *const kDateFormat = "yyyy-MM-ddTHH:mm:ss";

const char *const kSelectTransaction =
    "SELECT t.id, t.account_id, t.type, t.category_id, c.name, t.amount, t.description, t.occurred_at "
    "FROM transactions t "
    "JOIN accounts a ON a.id = t.account_id "
    "JOIN categories c ON c.id = t.category_id ";

Transaction readTransaction(const QSqlQuery &q)
{
    Transaction t;
    t.id = q.value(0).toLongLong();
    t.accountId = q.value(1).toLongLong();
    t.type = transactionTypeFromDb(q.value(2).toString());
    t.categoryId = q.value(3).toLongLong();
    t.categoryName = q.value(4).toString();
    t.amount = q.value(5).toLongLong();
    t.description = q.value(6).toString();
    t.occurredAt = QDateTime::fromString(q.value(7).toString(), Qt::ISODate).toLocalTime();
    return t;
}

bool fail(const QSqlQuery &q, QString *error)
{
    if (error)
        *error = q.lastError().text();
    return false;
}

} // namespace

QList<Transaction> TransactionRepository::listForAccount(qint64 accountId, qint64 userId)
{
    QList<Transaction> result;
    QSqlQuery q;
    q.prepare(QString(kSelectTransaction)
              + "WHERE t.account_id = ? AND a.user_id = ? ORDER BY t.occurred_at DESC, t.id DESC");
    q.addBindValue(accountId);
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next())
            result.append(readTransaction(q));
    }

    const auto tags = TagRepository::namesForAccount(accountId, userId);
    for (Transaction &t : result)
        t.tags = tags.value(t.id);
    return result;
}

std::optional<Transaction> TransactionRepository::find(qint64 transactionId, qint64 userId)
{
    QSqlQuery q;
    q.prepare(QString(kSelectTransaction) + "WHERE t.id = ? AND a.user_id = ?");
    q.addBindValue(transactionId);
    q.addBindValue(userId);
    if (!q.exec() || !q.next())
        return std::nullopt;
    Transaction t = readTransaction(q);
    t.tags = TagRepository::namesForTransaction(t.id);
    return t;
}

std::optional<qint64> TransactionRepository::insert(const Transaction &t, QString *error)
{
    QSqlQuery q;
    q.prepare("INSERT INTO transactions (account_id, category_id, amount, type, description, occurred_at) "
              "VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(t.accountId);
    q.addBindValue(t.categoryId);
    q.addBindValue(t.amount);
    q.addBindValue(toDbString(t.type));
    q.addBindValue(t.description.isEmpty() ? QVariant() : QVariant(t.description));
    q.addBindValue(t.occurredAt.toLocalTime().toString(kDateFormat));
    if (!q.exec()) {
        fail(q, error);
        return std::nullopt;
    }
    return q.lastInsertId().toLongLong();
}

bool TransactionRepository::update(const Transaction &t, qint64 userId, QString *error)
{
    QSqlQuery q;
    q.prepare("UPDATE transactions SET category_id = ?, amount = ?, type = ?, description = ?, occurred_at = ? "
              "WHERE id = ? AND account_id IN (SELECT id FROM accounts WHERE user_id = ?)");
    q.addBindValue(t.categoryId);
    q.addBindValue(t.amount);
    q.addBindValue(toDbString(t.type));
    q.addBindValue(t.description.isEmpty() ? QVariant() : QVariant(t.description));
    q.addBindValue(t.occurredAt.toLocalTime().toString(kDateFormat));
    q.addBindValue(t.id);
    q.addBindValue(userId);
    if (!q.exec())
        return fail(q, error);
    return q.numRowsAffected() == 1;
}

bool TransactionRepository::remove(qint64 transactionId, qint64 userId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM transactions "
              "WHERE id = ? AND account_id IN (SELECT id FROM accounts WHERE user_id = ?)");
    q.addBindValue(transactionId);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
}
