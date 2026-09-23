#include "db/TagRepository.h"

#include <QSqlQuery>
#include <QVariant>

QStringList TagRepository::namesForUser(qint64 userId)
{
    QStringList result;
    QSqlQuery q;
    q.prepare("SELECT name FROM tags WHERE user_id = ? ORDER BY name COLLATE NOCASE");
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next())
            result.append(q.value(0).toString());
    }
    return result;
}

std::optional<qint64> TagRepository::findOrCreate(qint64 userId, const QString &name)
{
    QSqlQuery q;
    q.prepare("SELECT id FROM tags WHERE user_id = ? AND name = ?");
    q.addBindValue(userId);
    q.addBindValue(name);
    if (q.exec() && q.next())
        return q.value(0).toLongLong();

    q.prepare("INSERT INTO tags (user_id, name) VALUES (?, ?)");
    q.addBindValue(userId);
    q.addBindValue(name);
    if (!q.exec())
        return std::nullopt;
    return q.lastInsertId().toLongLong();
}

bool TagRepository::setForTransaction(qint64 transactionId, const QList<qint64> &tagIds)
{
    QSqlQuery q;
    q.prepare("DELETE FROM transaction_tags WHERE transaction_id = ?");
    q.addBindValue(transactionId);
    if (!q.exec())
        return false;

    q.prepare("INSERT INTO transaction_tags (transaction_id, tag_id) VALUES (?, ?)");
    for (const qint64 tagId : tagIds) {
        q.addBindValue(transactionId);
        q.addBindValue(tagId);
        if (!q.exec())
            return false;
    }
    return true;
}

QHash<qint64, QStringList> TagRepository::namesForAccount(qint64 accountId, qint64 userId)
{
    QHash<qint64, QStringList> result;
    QSqlQuery q;
    q.prepare("SELECT tt.transaction_id, g.name FROM transaction_tags tt "
              "JOIN tags g ON g.id = tt.tag_id "
              "JOIN transactions t ON t.id = tt.transaction_id "
              "JOIN accounts a ON a.id = t.account_id "
              "WHERE t.account_id = ? AND a.user_id = ? "
              "ORDER BY g.name COLLATE NOCASE");
    q.addBindValue(accountId);
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next())
            result[q.value(0).toLongLong()].append(q.value(1).toString());
    }
    return result;
}

QStringList TagRepository::namesForTransaction(qint64 transactionId)
{
    QStringList result;
    QSqlQuery q;
    q.prepare("SELECT g.name FROM transaction_tags tt JOIN tags g ON g.id = tt.tag_id "
              "WHERE tt.transaction_id = ? ORDER BY g.name COLLATE NOCASE");
    q.addBindValue(transactionId);
    if (q.exec()) {
        while (q.next())
            result.append(q.value(0).toString());
    }
    return result;
}

void TagRepository::deleteUnused(qint64 userId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM tags WHERE user_id = ? "
              "AND id NOT IN (SELECT DISTINCT tag_id FROM transaction_tags)");
    q.addBindValue(userId);
    q.exec();
}
