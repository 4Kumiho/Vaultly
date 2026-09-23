#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

// Primitive crittografiche. AES-256-GCM e generatore casuale dalle API CNG di Windows (bcrypt).
namespace Crypto {

constexpr int kKeyBytes = 32;

// Byte casuali dal generatore crittografico del sistema.
QByteArray randomBytes(int count);

// PBKDF2-SHA256 → chiave da kKeyBytes.
QByteArray deriveKey(const QString &password, const QByteArray &salt, int iterations);

// Cifra e autentica. Risultato: nonce (12 byte) | testo cifrato | tag (16 byte).
std::optional<QByteArray> encrypt(const QByteArray &key, const QByteArray &plaintext);

// nullopt se la chiave è sbagliata o i dati sono stati alterati.
std::optional<QByteArray> decrypt(const QByteArray &key, const QByteArray &blob);

// Confronto a tempo costante, per non rivelare quanti byte iniziali coincidono.
bool constantTimeEquals(const QByteArray &a, const QByteArray &b);

} // namespace Crypto
