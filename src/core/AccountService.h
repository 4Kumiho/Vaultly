#pragma once

#include "core/Account.h"

#include <optional>

namespace AccountService {

struct Result
{
    std::optional<Account> account; // valorizzato solo in caso di successo
    QString error;                  // messaggio da mostrare all'utente
};

Result create(qint64 userId, const QString &name, const Currency &currency, qint64 initialBalance);

// La valuta non si cambia: i movimenti esistenti sono già in quella valuta.
Result update(const Account &account, const QString &name, qint64 initialBalance);

// Elimina il conto con tutti i suoi movimenti.
bool remove(const Account &account);

} // namespace AccountService
