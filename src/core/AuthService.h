#pragma once

#include "core/User.h"

#include <optional>

namespace AuthService {

constexpr int kMinPasswordLength = 6;

struct Result
{
    std::optional<User> user; // valorizzato solo in caso di successo
    QByteArray vaultKey;      // chiave dell'area password, derivata dalla password appena inserita
    QString error;            // messaggio da mostrare all'utente

    Session session() const { return {user.value_or(User()), vaultKey}; }
};

Result registerUser(const QString &username, const QString &password, const QString &confirmPassword);
Result login(const QString &username, const QString &password);

} // namespace AuthService
