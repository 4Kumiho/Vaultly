#pragma once

#include <QDateTime>
#include <QString>

// Una voce dell'area password. Nel DB viene salvata tutta cifrata (vedi VaultService).
struct VaultEntry
{
    qint64 id = 0;
    QString title;    // es. "Gmail"
    QString url;      // facoltativo
    QString username; // username o email
    QString password;
    QString notes;    // facoltative
    QDateTime updatedAt;
};
