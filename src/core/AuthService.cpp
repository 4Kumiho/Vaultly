#include "core/AuthService.h"

#include "core/Crypto.h"
#include "db/UserRepository.h"

#include <QCoreApplication>

namespace {

constexpr int kIterations = 100000;
constexpr int kSaltBytes = 16;

AuthService::Result failure(const QString &message)
{
    return {std::nullopt, {}, message};
}

// La chiave dell'area password usa un salt diverso da quello dell'hash di login:
// così l'hash salvato nel DB non serve a decifrare nulla.
QByteArray vaultKeyFor(User &user, const QString &password)
{
    if (user.vaultSalt.isEmpty()) {
        // Utente creato prima dell'area password: il salt si genera al primo login.
        user.vaultSalt = Crypto::randomBytes(kSaltBytes);
        UserRepository::setVaultSalt(user.id, user.vaultSalt);
    }
    return Crypto::deriveKey(password, user.vaultSalt, kIterations);
}

} // namespace

AuthService::Result AuthService::registerUser(const QString &username, const QString &password,
                                              const QString &confirmPassword)
{
    const QString name = username.trimmed();
    if (name.isEmpty())
        return failure(QCoreApplication::translate("AuthService", "Inserisci uno username."));
    if (password.size() < kMinPasswordLength)
        return failure(QCoreApplication::translate("AuthService", "La password deve avere almeno %1 caratteri.")
                           .arg(kMinPasswordLength));
    if (password != confirmPassword)
        return failure(QCoreApplication::translate("AuthService", "Le password non coincidono."));
    if (UserRepository::findByUsername(name))
        return failure(QCoreApplication::translate("AuthService", "Username già in uso."));

    User user;
    user.username = name;
    user.salt = Crypto::randomBytes(kSaltBytes);
    user.iterations = kIterations;
    user.passwordHash = Crypto::deriveKey(password, user.salt, user.iterations);
    user.vaultSalt = Crypto::randomBytes(kSaltBytes);

    const auto id = UserRepository::insert(user);
    if (!id)
        return failure(QCoreApplication::translate("AuthService", "Impossibile creare l'utente."));
    user.id = *id;
    const QByteArray key = vaultKeyFor(user, password);
    return {user, key, {}};
}

AuthService::Result AuthService::login(const QString &username, const QString &password)
{
    // Stesso messaggio per utente inesistente e password errata.
    const QString wrongCredentials =
        QCoreApplication::translate("AuthService", "Username o password errati.");

    auto user = UserRepository::findByUsername(username.trimmed());
    if (!user)
        return failure(wrongCredentials);
    if (!Crypto::constantTimeEquals(Crypto::deriveKey(password, user->salt, user->iterations), user->passwordHash))
        return failure(wrongCredentials);
    const QByteArray key = vaultKeyFor(*user, password);
    return {user, key, {}};
}
