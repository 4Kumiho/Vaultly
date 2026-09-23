#include "core/Crypto.h"

#include <QTest>

class TestCrypto : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip()
    {
        const QByteArray key = Crypto::randomBytes(Crypto::kKeyBytes);
        const QByteArray plain = QString("password segreta · àèìòù").toUtf8();
        const auto blob = Crypto::encrypt(key, plain);
        QVERIFY(blob);
        QVERIFY(!blob->contains(plain));
        QCOMPARE(Crypto::decrypt(key, *blob).value_or(QByteArray()), plain);
    }

    void sameInputGivesDifferentCiphertext()
    {
        const QByteArray key = Crypto::randomBytes(Crypto::kKeyBytes);
        QVERIFY(*Crypto::encrypt(key, "ciao") != *Crypto::encrypt(key, "ciao"));
    }

    void wrongKeyFails()
    {
        const auto blob = Crypto::encrypt(Crypto::randomBytes(Crypto::kKeyBytes), "ciao");
        QVERIFY(blob);
        QVERIFY(!Crypto::decrypt(Crypto::randomBytes(Crypto::kKeyBytes), *blob));
    }

    void tamperingIsDetected()
    {
        const QByteArray key = Crypto::randomBytes(Crypto::kKeyBytes);
        QByteArray blob = *Crypto::encrypt(key, "importo: 100");
        blob[14] = char(blob[14] ^ 0x01);
        QVERIFY(!Crypto::decrypt(key, blob));
        QVERIFY(!Crypto::decrypt(key, blob.left(10)));
    }

    void rejectsBadKeySize() { QVERIFY(!Crypto::encrypt("corta", "ciao")); }

    void deriveKeyIsDeterministic()
    {
        const QByteArray salt = Crypto::randomBytes(16);
        QCOMPARE(Crypto::deriveKey("pw", salt, 1000), Crypto::deriveKey("pw", salt, 1000));
        QVERIFY(Crypto::deriveKey("pw", salt, 1000) != Crypto::deriveKey("pw2", salt, 1000));
        QCOMPARE(Crypto::deriveKey("pw", salt, 1000).size(), Crypto::kKeyBytes);
    }
};

QTEST_GUILESS_MAIN(TestCrypto)
#include "tst_crypto.moc"
