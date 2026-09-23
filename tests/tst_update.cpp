#include "core/Appcast.h"
#include "core/Crypto.h"

#include <QTest>

namespace {

QByteArray hex(const char *text)
{
    return QByteArray::fromHex(text);
}

// Stesso formato scritto da scripts/release.ps1.
// Niente raw string qui: il preprocessore di moc non le digerisce e salterebbe la classe di test.
QByteArray appcastItem(const QString &version, const QString &url = {}, const QString &os = "windows-x64",
                       const QString &length = "15132273")
{
    const QString u = url.isEmpty() ? QString("https://github.com/4Kumiho/Vaultly/releases/download/v%1/"
                                              "Vaultly-Setup-%1.exe")
                                          .arg(version)
                                    : url;
    const QString signature = QString(QByteArray(64, 'x').toBase64());
    return QString("<item><title>Vaultly %1</title>"
                   "<description><![CDATA[<ul><li>Novita %1</li></ul>]]></description>"
                   "<enclosure url='%2' sparkle:version='%1' sparkle:os='%3'"
                   " sparkle:installerArguments='/SILENT /SP- /NOCANCEL'"
                   " sparkle:edSignature='%4' length='%5' type='application/octet-stream' /></item>")
        .arg(version, u, os, signature, length)
        .toUtf8();
}

QByteArray appcast(const QByteArray &items)
{
    return "<?xml version='1.0' encoding='utf-8'?>"
           "<rss version='2.0' xmlns:sparkle='http://www.andymatuschak.org/xml-namespaces/sparkle'>"
           "<channel><title>Vaultly</title>"
        + items + "</channel></rss>";
}

} // namespace

class TestUpdate : public QObject
{
    Q_OBJECT

private slots:
    void parsesReleaseScriptFormat()
    {
        const auto info = Appcast::parse(appcast(appcastItem("1.0.3")));
        QVERIFY(info);
        QCOMPARE(info->version, QString("1.0.3"));
        QCOMPARE(info->url.toString(),
                 QString("https://github.com/4Kumiho/Vaultly/releases/download/v1.0.3/Vaultly-Setup-1.0.3.exe"));
        QCOMPARE(info->signature, QByteArray(64, 'x'));
        QCOMPARE(info->length, qint64(15132273));
        QCOMPARE(info->notesHtml, QString("<ul><li>Novita 1.0.3</li></ul>"));
    }

    void picksNewestItem()
    {
        const auto info = Appcast::parse(appcast(appcastItem("1.0.9") + appcastItem("1.0.10") + appcastItem("1.0.2")));
        QVERIFY(info);
        QCOMPARE(info->version, QString("1.0.10"));
    }

    void ignoresUnsafeOrForeignItems()
    {
        QVERIFY(!Appcast::parse(appcast(appcastItem("2.0.0", "http://example.com/setup.exe"))));
        QVERIFY(!Appcast::parse(appcast(appcastItem("2.0.0", {}, "macos"))));
        QVERIFY(!Appcast::parse(appcast(appcastItem("2.0.0", {}, "windows-x64", "0"))));
        QVERIFY(!Appcast::parse(appcast(appcastItem("2.0.0", {}, "windows-x64", "999999999999"))));
        // Una voce valida accanto a una non valida: vince quella valida.
        const auto info =
            Appcast::parse(appcast(appcastItem("3.0.0", "http://x/setup.exe") + appcastItem("1.0.5")));
        QVERIFY(info);
        QCOMPARE(info->version, QString("1.0.5"));
    }

    void rejectsGarbage()
    {
        QVERIFY(!Appcast::parse("non è xml"));
        QVERIFY(!Appcast::parse(appcast({})));
    }

    void comparesVersions()
    {
        QVERIFY(Appcast::compareVersions("1.0.10", "1.0.9") > 0);
        QVERIFY(Appcast::compareVersions("1.0.2", "1.0.3") < 0);
        QVERIFY(Appcast::compareVersions("2.0", "1.9.9") > 0);
        QCOMPARE(Appcast::compareVersions("1.0", "1.0.0"), 0);
    }

    void verifiesEd25519Rfc8032Vectors()
    {
        // RFC 8032, sezione 7.1, TEST 1 (messaggio vuoto) e TEST 2 (un byte).
        QVERIFY(Crypto::verifyEd25519(
            hex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"),
            hex("e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25b"
                "f5f0595bbe24655141438e7a100b"),
            QByteArray()));
        QVERIFY(Crypto::verifyEd25519(
            hex("3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c"),
            hex("92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b"
                "2eaeb4302aeeb00d291612bb0c00"),
            hex("72")));
    }

    void verifiesWinSparkleToolSignature()
    {
        // Firma prodotta da `winsparkle-tool sign` con una chiave usa e getta (non quella delle release).
        const QByteArray publicKey = QByteArray::fromBase64("uLhOLVG9HD8NVHCMWM74AIRIqoxxeHvaIvulliaNzmo=");
        const QByteArray signature = QByteArray::fromBase64(
            "oYKvf1fNKAG5ri8hdM24dUnegCVyazC1gKaN76lKWg2ar7vPN/f9O+i2VNuTa1F+Eb/t34xqs03I88FpyLpPDA==");
        const QByteArray message = "Vaultly: messaggio di prova per la firma degli aggiornamenti";
        QVERIFY(Crypto::verifyEd25519(publicKey, signature, message));

        QByteArray tampered = message;
        tampered[0] = 'v';
        QVERIFY(!Crypto::verifyEd25519(publicKey, signature, tampered));
        QVERIFY(!Crypto::verifyEd25519(QByteArray::fromBase64("YgjC4rRMQjdPz1CfZzWaX7JYjNnO7yXSm7nfsL0MNZ8="),
                                       signature, message));
        QVERIFY(!Crypto::verifyEd25519(publicKey, signature.left(63), message));
    }
};

QTEST_GUILESS_MAIN(TestUpdate)
#include "tst_update.moc"
