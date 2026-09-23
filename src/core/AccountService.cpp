#include "core/AccountService.h"

#include "db/AccountRepository.h"
#include "db/TagRepository.h"

#include <QCoreApplication>

namespace {

AccountService::Result failure(const QString &message)
{
    return {std::nullopt, message};
}

// Restituisce un messaggio d'errore, o una stringa vuota se il nome va bene.
QString validateName(qint64 userId, const QString &name, qint64 excludeAccountId)
{
    if (name.isEmpty())
        return QCoreApplication::translate("AccountService", "Inserisci un nome per il conto.");
    if (AccountRepository::nameExists(userId, name, excludeAccountId))
        return QCoreApplication::translate("AccountService", "Hai già un conto con questo nome.");
    return {};
}

} // namespace

AccountService::Result AccountService::create(qint64 userId, const QString &name, const Currency &currency,
                                              qint64 initialBalance)
{
    const QString trimmed = name.trimmed();
    if (const QString error = validateName(userId, trimmed, 0); !error.isEmpty())
        return failure(error);

    const auto id = AccountRepository::insert(userId, trimmed, currency.code, initialBalance);
    if (!id)
        return failure(QCoreApplication::translate("AccountService", "Impossibile creare il conto."));

    Account account;
    account.id = *id;
    account.userId = userId;
    account.name = trimmed;
    account.currency = currency;
    account.initialBalance = initialBalance;
    return {account, {}};
}

AccountService::Result AccountService::update(const Account &account, const QString &name, qint64 initialBalance)
{
    const QString trimmed = name.trimmed();
    if (const QString error = validateName(account.userId, trimmed, account.id); !error.isEmpty())
        return failure(error);

    if (!AccountRepository::update(account.id, account.userId, trimmed, initialBalance))
        return failure(QCoreApplication::translate("AccountService", "Impossibile modificare il conto."));

    Account updated = account;
    updated.name = trimmed;
    updated.initialBalance = initialBalance;
    return {updated, {}};
}

bool AccountService::remove(const Account &account)
{
    if (!AccountRepository::remove(account.id, account.userId))
        return false;
    // I movimenti se ne vanno a cascata: le etichette usate solo lì restano orfane.
    TagRepository::deleteUnused(account.userId);
    return true;
}
