#include "core/Crypto.h"

#include <QPasswordDigestor>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// windows.h deve precedere bcrypt.h
#include <bcrypt.h>

namespace {

constexpr ULONG kNonceBytes = 12;
constexpr ULONG kTagBytes = 16;

// Chiudono gli handle CNG all'uscita dallo scope.
struct Algorithm
{
    BCRYPT_ALG_HANDLE handle = nullptr;
    ~Algorithm()
    {
        if (handle)
            BCryptCloseAlgorithmProvider(handle, 0);
    }
};

struct Key
{
    BCRYPT_KEY_HANDLE handle = nullptr;
    ~Key()
    {
        if (handle)
            BCryptDestroyKey(handle);
    }
};

bool openAesGcm(Algorithm &alg, Key &key, const QByteArray &rawKey)
{
    if (rawKey.size() != Crypto::kKeyBytes)
        return false;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg.handle, BCRYPT_AES_ALGORITHM, nullptr, 0)))
        return false;
    if (!BCRYPT_SUCCESS(BCryptSetProperty(alg.handle, BCRYPT_CHAINING_MODE,
                                          reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(BCRYPT_CHAIN_MODE_GCM)),
                                          sizeof(BCRYPT_CHAIN_MODE_GCM), 0)))
        return false;
    return BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(alg.handle, &key.handle, nullptr, 0,
                                                     reinterpret_cast<PUCHAR>(const_cast<char *>(rawKey.constData())),
                                                     ULONG(rawKey.size()), 0));
}

PUCHAR bytes(QByteArray &data)
{
    return reinterpret_cast<PUCHAR>(data.data());
}

} // namespace

QByteArray Crypto::randomBytes(int count)
{
    QByteArray out(count, Qt::Uninitialized);
    if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, bytes(out), ULONG(count), BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
        qFatal("BCryptGenRandom non disponibile");
    return out;
}

QByteArray Crypto::deriveKey(const QString &password, const QByteArray &salt, int iterations)
{
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, password.toUtf8(), salt, iterations,
                                              kKeyBytes);
}

std::optional<QByteArray> Crypto::encrypt(const QByteArray &key, const QByteArray &plaintext)
{
    Algorithm alg;
    Key k;
    if (!openAesGcm(alg, k, key))
        return std::nullopt;

    QByteArray nonce = randomBytes(kNonceBytes);
    QByteArray tag(kTagBytes, '\0');
    QByteArray input = plaintext;
    QByteArray cipher(plaintext.size(), '\0');

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
    BCRYPT_INIT_AUTH_MODE_INFO(info);
    info.pbNonce = bytes(nonce);
    info.cbNonce = kNonceBytes;
    info.pbTag = bytes(tag);
    info.cbTag = kTagBytes;

    ULONG written = 0;
    if (!BCRYPT_SUCCESS(BCryptEncrypt(k.handle, bytes(input), ULONG(input.size()), &info, nullptr, 0, bytes(cipher),
                                      ULONG(cipher.size()), &written, 0)))
        return std::nullopt;
    return nonce + cipher + tag;
}

std::optional<QByteArray> Crypto::decrypt(const QByteArray &key, const QByteArray &blob)
{
    if (blob.size() < qsizetype(kNonceBytes + kTagBytes))
        return std::nullopt;

    Algorithm alg;
    Key k;
    if (!openAesGcm(alg, k, key))
        return std::nullopt;

    QByteArray nonce = blob.left(kNonceBytes);
    QByteArray tag = blob.right(kTagBytes);
    QByteArray cipher = blob.mid(kNonceBytes, blob.size() - kNonceBytes - kTagBytes);
    QByteArray plain(cipher.size(), '\0');

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
    BCRYPT_INIT_AUTH_MODE_INFO(info);
    info.pbNonce = bytes(nonce);
    info.cbNonce = kNonceBytes;
    info.pbTag = bytes(tag);
    info.cbTag = kTagBytes;

    ULONG written = 0;
    // Con chiave sbagliata o dati alterati il tag non corrisponde e BCryptDecrypt fallisce.
    if (!BCRYPT_SUCCESS(BCryptDecrypt(k.handle, bytes(cipher), ULONG(cipher.size()), &info, nullptr, 0, bytes(plain),
                                      ULONG(plain.size()), &written, 0)))
        return std::nullopt;
    return plain;
}

bool Crypto::constantTimeEquals(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size())
        return false;
    unsigned char diff = 0;
    for (qsizetype i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}
