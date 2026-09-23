#pragma once

#include <QByteArray>
#include <QString>

struct User
{
    qint64 id = 0;
    QString username;
    QByteArray passwordHash;
    QByteArray salt;
    int iterations = 0;
    QByteArray vaultSalt; // salt per la chiave dell'area password; vuoto per utenti creati prima della v2
};

// Utente connesso. La chiave dell'area password esiste solo qui, in memoria, finché dura la sessione.
struct Session
{
    User user;
    QByteArray vaultKey;
};
