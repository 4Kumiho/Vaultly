#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>

#include <optional>

// Accesso grezzo alla tabella vault_entries: vede solo blob cifrati.
// Cifratura e decifratura stanno in VaultService.
namespace VaultRepository {

struct Row
{
    qint64 id = 0;
    QByteArray data;
    QDateTime updatedAt;
};

QList<Row> listForUser(qint64 userId);

std::optional<qint64> insert(qint64 userId, const QByteArray &data);

bool update(qint64 entryId, qint64 userId, const QByteArray &data);

bool remove(qint64 entryId, qint64 userId);

} // namespace VaultRepository
