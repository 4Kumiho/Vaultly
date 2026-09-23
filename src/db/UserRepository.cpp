#include "db/UserRepository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

std::optional<User> UserRepository::findByUsername(const QString &username)
{
    QSqlQuery q;
    q.prepare("SELECT id, username, password_hash, salt, iterations, vault_salt FROM users WHERE username = ?");
    q.addBindValue(username);
    if (!q.exec() || !q.next())
        return std::nullopt;

    User user;
    user.id = q.value(0).toLongLong();
    user.username = q.value(1).toString();
    user.passwordHash = q.value(2).toByteArray();
    user.salt = q.value(3).toByteArray();
    user.iterations = q.value(4).toInt();
    user.vaultSalt = q.value(5).toByteArray();
    return user;
}

std::optional<qint64> UserRepository::insert(const User &user, QString *error)
{
    QSqlQuery q;
    q.prepare("INSERT INTO users (username, password_hash, salt, iterations, vault_salt, created_at) "
              "VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(user.username);
    q.addBindValue(user.passwordHash);
    q.addBindValue(user.salt);
    q.addBindValue(user.iterations);
    q.addBindValue(user.vaultSalt);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return std::nullopt;
    }
    return q.lastInsertId().toLongLong();
}

bool UserRepository::setVaultSalt(qint64 userId, const QByteArray &vaultSalt)
{
    QSqlQuery q;
    q.prepare("UPDATE users SET vault_salt = ? WHERE id = ?");
    q.addBindValue(vaultSalt);
    q.addBindValue(userId);
    return q.exec() && q.numRowsAffected() == 1;
}
