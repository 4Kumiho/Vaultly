#pragma once

#include "core/User.h"

#include <optional>

namespace UserRepository {

// Ricerca case-insensitive (la colonna username è COLLATE NOCASE).
std::optional<User> findByUsername(const QString &username);

// Restituisce l'id del nuovo utente, o nullopt se l'inserimento fallisce.
std::optional<qint64> insert(const User &user, QString *error = nullptr);

bool setVaultSalt(qint64 userId, const QByteArray &vaultSalt);

} // namespace UserRepository
