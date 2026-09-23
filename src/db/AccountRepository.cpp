#include "db/AccountRepository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

const char *const kSelectAccount =
    "SELECT a.id, a.user_id, a.name, a.initial_balance, c.code, c.symbol, c.minor_units "
    "FROM accounts a JOIN currencies c ON c.code = a.currency_code ";

Account readAccount(const QSqlQuery &q)
{
    Account a;
    a.id = q.value(0).toLongLong();
    a.userId = q.value(1).toLongLong();
    a.name = q.value(2).toString();
    a.initialBalance = q.value(3).toLongLong();
    a.currency = {q.value(4).toString(), q.value(5).toString(), q.value(6).toInt()};
    return a;
}

} // namespace

QList<Account> AccountRepository::listForUser(qint64 userId)
{
    QList<Account> result;
    QSqlQuery q;
    q.prepare(QString(kSelectAccount) + "WHERE a.user_id = ? ORDER BY a.name COLLATE NOCASE");
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next())
            result.append(readAccount(q));
    }
    return result;
}

std::optional<Account> AccountRepository::find(qint64 accountId, qint64 userId)
{
    QSqlQuery q;
    q.prepare(QString(kSelectAccount) + "WHERE a.id = ? AND a.user_id = ?");
    q.addBindValue(accountId);
    q.addBindValue(userId);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return readAccount(q);
}

bool AccountRepository::nameExists(qint64 userId, const QString &name, qint64 excludeAccountId)
{
    QSqlQuery q;
    q.prepare("SELECT 1 FROM accounts WHERE user_id = ? AND name = ? COLLATE NOCASE AND id != ?");
    q.addBindValue(userId);
    q.addBindValue(name);
    q.addBindValue(excludeAccountId);
    return q.exec() && q.next();
}

std::optional<qint64> AccountRepository::insert(qint64 userId, const QString &name, const QString &currencyCode,
                                                qint64 initialBalance, QString *error)
{
    QSqlQuery q;
    q.prepare("INSERT INTO accounts (user_id, name, currency_code, initial_balance, created_at) "
              "VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(userId);
    q.addBindValue(name);
    q.addBindValue(currencyCode);
    q.addBindValue(initialBalance);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return std::nullopt;
    }
    return q.lastInsertId().toLongLong();
}

bool AccountRepository::update(qint64 accountId, qint64 userId, const QString &name, qint64 initialBalance,
                               QString *error)
{
    QSqlQuery q;
    q.prepare("UPDATE accounts SET name = ?, initial_balance = ? WHERE id = ? AND user_id = ?");
    q.addBindValue(name);
    q.addBindValue(initialBalance);
    q.addBindValue(accountId);
    q.addBindValue(userId);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool AccountRepository::remove(qint64 accountId, qint64 userId)
{
    // I movimenti se ne vanno con ON DELETE CASCADE (foreign key attive in Database::open).
    QSqlQuery q;
    q.prepare("DELETE FROM accounts WHERE id = ? AND user_id = ?");
    q.addBindValue(accountId);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
}

int AccountRepository::transactionCount(qint64 accountId, qint64 userId)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM transactions t JOIN accounts a ON a.id = t.account_id "
              "WHERE a.id = ? AND a.user_id = ?");
    q.addBindValue(accountId);
    q.addBindValue(userId);
    if (!q.exec() || !q.next())
        return 0;
    return q.value(0).toInt();
}

qint64 AccountRepository::currentBalance(qint64 accountId, qint64 userId)
{
    QSqlQuery q;
    q.prepare("SELECT a.initial_balance + COALESCE(SUM(CASE t.type WHEN 'income' THEN t.amount "
              "ELSE -t.amount END), 0) "
              "FROM accounts a LEFT JOIN transactions t ON t.account_id = a.id "
              "WHERE a.id = ? AND a.user_id = ? GROUP BY a.id");
    q.addBindValue(accountId);
    q.addBindValue(userId);
    if (!q.exec() || !q.next())
        return 0;
    return q.value(0).toLongLong();
}
