#include "db/VaultRepository.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

QString nowUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

} // namespace

QList<VaultRepository::Row> VaultRepository::listForUser(qint64 userId)
{
    QList<Row> result;
    QSqlQuery q;
    q.prepare("SELECT id, data, updated_at FROM vault_entries WHERE user_id = ?");
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next()) {
            result.append({q.value(0).toLongLong(), q.value(1).toByteArray(),
                           QDateTime::fromString(q.value(2).toString(), Qt::ISODate).toLocalTime()});
        }
    }
    return result;
}

std::optional<qint64> VaultRepository::insert(qint64 userId, const QByteArray &data)
{
    QSqlQuery q;
    q.prepare("INSERT INTO vault_entries (user_id, data, created_at, updated_at) VALUES (?, ?, ?, ?)");
    q.addBindValue(userId);
    q.addBindValue(data);
    q.addBindValue(nowUtc());
    q.addBindValue(nowUtc());
    if (!q.exec())
        return std::nullopt;
    return q.lastInsertId().toLongLong();
}

bool VaultRepository::update(qint64 entryId, qint64 userId, const QByteArray &data)
{
    QSqlQuery q;
    q.prepare("UPDATE vault_entries SET data = ?, updated_at = ? WHERE id = ? AND user_id = ?");
    q.addBindValue(data);
    q.addBindValue(nowUtc());
    q.addBindValue(entryId);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
}

bool VaultRepository::remove(qint64 entryId, qint64 userId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM vault_entries WHERE id = ? AND user_id = ?");
    q.addBindValue(entryId);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
}
