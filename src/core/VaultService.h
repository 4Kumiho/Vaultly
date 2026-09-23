#pragma once

#include "core/User.h"
#include "core/VaultEntry.h"

#include <QList>

#include <optional>

// Area password: ogni voce è serializzata in JSON e cifrata con la chiave della sessione.
namespace VaultService {

struct Result
{
    std::optional<VaultEntry> entry; // valorizzato solo in caso di successo
    QString error;                   // messaggio da mostrare all'utente
};

// Voci dell'utente, ordinate per nome. Quelle che non si riescono a decifrare
// (dati alterati) vengono saltate e contate in `unreadable`.
QList<VaultEntry> list(const Session &session, int *unreadable = nullptr);

Result create(const Session &session, const VaultEntry &entry);
Result update(const Session &session, const VaultEntry &entry);
bool remove(const Session &session, qint64 entryId);

// Password casuale con almeno una minuscola, una maiuscola, una cifra e (se richiesto) un simbolo.
QString generatePassword(int length = 20, bool symbols = true);

} // namespace VaultService
