#include "db/TagRepository.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

// Stesso formato di transactions.occurred_at: confrontabile come stringa.
const char *const kDateTimeFormat = "yyyy-MM-ddTHH:mm:ss";
const char *const kDateFormat = "yyyy-MM-dd";

const char *const kSelectTag =
    "SELECT g.id, g.name, g.period, g.start_date, g.end_date, g.budget, g.currency_code, "
    "       (SELECT COUNT(*) FROM transaction_tags tt WHERE tt.tag_id = g.id) "
    "FROM tags g ";

Tag readTag(const QSqlQuery &q)
{
    Tag t;
    t.id = q.value(0).toLongLong();
    t.name = q.value(1).toString();
    t.period = tagPeriodFromDb(q.value(2).toString());
    t.start = QDate::fromString(q.value(3).toString(), kDateFormat);
    t.end = QDate::fromString(q.value(4).toString(), kDateFormat);
    if (!q.value(5).isNull())
        t.budget = q.value(5).toLongLong();
    t.currencyCode = q.value(6).toString();
    t.usage = q.value(7).toInt();
    return t;
}

QVariant dateOrNull(const QDate &date)
{
    return date.isValid() ? QVariant(date.toString(kDateFormat)) : QVariant();
}

void bindSettings(QSqlQuery &q, const Tag &t)
{
    q.addBindValue(t.name);
    q.addBindValue(toDbString(t.period));
    q.addBindValue(dateOrNull(t.start));
    q.addBindValue(dateOrNull(t.end));
    q.addBindValue(t.budget ? QVariant(*t.budget) : QVariant());
    q.addBindValue(t.currencyCode.isEmpty() ? QVariant() : QVariant(t.currencyCode));
}

} // namespace

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

QList<Tag> TagRepository::listForUser(qint64 userId)
{
    QList<Tag> result;
    QSqlQuery q;
    q.prepare(QString(kSelectTag) + "WHERE g.user_id = ? ORDER BY g.name COLLATE NOCASE");
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next())
            result.append(readTag(q));
    }
    return result;
}

std::optional<Tag> TagRepository::find(qint64 tagId, qint64 userId)
{
    QSqlQuery q;
    q.prepare(QString(kSelectTag) + "WHERE g.id = ? AND g.user_id = ?");
    q.addBindValue(tagId);
    q.addBindValue(userId);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return readTag(q);
}

bool TagRepository::nameExists(qint64 userId, const QString &name, qint64 excludeTagId)
{
    QSqlQuery q;
    q.prepare("SELECT 1 FROM tags WHERE user_id = ? AND name = ? AND id != ?");
    q.addBindValue(userId);
    q.addBindValue(name);
    q.addBindValue(excludeTagId);
    return q.exec() && q.next();
}

std::optional<qint64> TagRepository::insert(qint64 userId, const Tag &tag)
{
    QSqlQuery q;
    q.prepare("INSERT INTO tags (name, period, start_date, end_date, budget, currency_code, user_id) "
              "VALUES (?, ?, ?, ?, ?, ?, ?)");
    bindSettings(q, tag);
    q.addBindValue(userId);
    if (!q.exec())
        return std::nullopt;
    return q.lastInsertId().toLongLong();
}

bool TagRepository::update(const Tag &tag, qint64 userId)
{
    QSqlQuery q;
    q.prepare("UPDATE tags SET name = ?, period = ?, start_date = ?, end_date = ?, budget = ?, currency_code = ? "
              "WHERE id = ? AND user_id = ?");
    bindSettings(q, tag);
    q.addBindValue(tag.id);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
}

bool TagRepository::remove(qint64 tagId, qint64 userId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM tags WHERE id = ? AND user_id = ?");
    q.addBindValue(tagId);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
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

qint64 TagRepository::spent(qint64 userId, qint64 tagId, const QString &currencyCode, const QDateTime &from,
                            const QDateTime &to)
{
    QSqlQuery q;
    q.prepare("SELECT COALESCE(SUM(t.amount), 0) FROM transactions t "
              "JOIN transaction_tags tt ON tt.transaction_id = t.id "
              "JOIN accounts a ON a.id = t.account_id "
              "WHERE tt.tag_id = ? AND a.user_id = ? AND a.currency_code = ? AND t.type = 'expense' "
              "AND t.occurred_at >= ? AND t.occurred_at <= ?");
    q.addBindValue(tagId);
    q.addBindValue(userId);
    q.addBindValue(currencyCode);
    q.addBindValue(from.isValid() ? from.toString(kDateTimeFormat) : QStringLiteral("0000"));
    q.addBindValue(to.toString(kDateTimeFormat));
    if (!q.exec() || !q.next())
        return 0;
    return q.value(0).toLongLong();
}
