#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

// Primitive crittografiche. AES-256-GCM e generatore casuale dalle API CNG di Windows (bcrypt),
// verifica Ed25519 da Monocypher.
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

// Verifica una firma Ed25519 (RFC 8032) di `message`. Chiave 32 byte, firma 64 byte.
// Usata per gli aggiornamenti: le firme le produce winsparkle-tool (vedi scripts/release.ps1).
bool verifyEd25519(const QByteArray &publicKey, const QByteArray &signature, const QByteArray &message);

// Confronto a tempo costante, per non rivelare quanti byte iniziali coincidono.
bool constantTimeEquals(const QByteArray &a, const QByteArray &b);

} // namespace Crypto
