#include "core/AuthService.h"
#include "core/VaultService.h"
#include "db/Database.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

namespace {

VaultEntry makeEntry(const QString &title, const QString &password = "Sup3r-Segreta!")
{
    VaultEntry e;
    e.title = title;
    e.url = "https://example.com";
    e.username = "mario@example.com";
    e.password = password;
    e.notes = "domanda di sicurezza: gatto";
    return e;
}

} // namespace

class TestVault : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(Database::open(":memory:"));
        m_mario = AuthService::registerUser("mario", "segreto1", "segreto1").session();
    }
    void cleanup() { Database::close(); }

    void loginGivesSameKeyAsRegistration()
    {
        QCOMPARE(m_mario.vaultKey.size(), 32);
        QCOMPARE(AuthService::login("mario", "segreto1").vaultKey, m_mario.vaultKey);
        // Chiave diversa dall'hash di login salvato nel DB.
        QVERIFY(m_mario.vaultKey != m_mario.user.passwordHash);
    }

    void createAndListRoundTrip()
    {
        QVERIFY(VaultService::create(m_mario, makeEntry("Netflix")).entry);
        const auto r = VaultService::create(m_mario, makeEntry("  gmail  "));
        QVERIFY2(r.entry, qPrintable(r.error));

        const auto entries = VaultService::list(m_mario);
        QCOMPARE(entries.size(), 2);
        QCOMPARE(entries[0].title, QString("gmail")); // ordine alfabetico, senza maiuscole
        QCOMPARE(entries[0].password, QString("Sup3r-Segreta!"));
        QCOMPARE(entries[0].notes, QString("domanda di sicurezza: gatto"));
        QCOMPARE(entries[1].title, QString("Netflix"));
    }

    void nothingIsStoredInClear()
    {
        QVERIFY(VaultService::create(m_mario, makeEntry("Gmail")).entry);
        QSqlQuery q("SELECT data FROM vault_entries");
        QVERIFY(q.next());
        const QByteArray blob = q.value(0).toByteArray();
        for (const char *secret : {"Gmail", "Sup3r-Segreta!", "mario@example.com", "example.com", "gatto"})
            QVERIFY2(!blob.contains(secret), secret);
    }

    void validation()
    {
        QVERIFY(!VaultService::create(m_mario, makeEntry("   ")).entry);
        VaultEntry empty = makeEntry("Gmail");
        empty.username.clear();
        empty.password.clear();
        QVERIFY(!VaultService::create(m_mario, empty).entry);
    }

    void updateAndRemove()
    {
        auto e = *VaultService::create(m_mario, makeEntry("Gmail")).entry;
        e.password = "nuova-password";
        QVERIFY(VaultService::update(m_mario, e).entry);
        QCOMPARE(VaultService::list(m_mario).first().password, QString("nuova-password"));

        QVERIFY(VaultService::remove(m_mario, e.id));
        QVERIFY(VaultService::list(m_mario).isEmpty());
    }

    void otherUsersSeeNothing()
    {
        auto e = *VaultService::create(m_mario, makeEntry("Gmail")).entry;
        const Session anna = AuthService::registerUser("anna", "segreto1", "segreto1").session();
        QVERIFY(anna.vaultKey != m_mario.vaultKey); // stessa password, salt diverso

        QVERIFY(VaultService::list(anna).isEmpty());
        e.password = "rubata";
        QVERIFY(!VaultService::update(anna, e).entry);
        QVERIFY(!VaultService::remove(anna, e.id));
        QCOMPARE(VaultService::list(m_mario).first().password, QString("Sup3r-Segreta!"));
    }

    void wrongKeyCannotRead()
    {
        QVERIFY(VaultService::create(m_mario, makeEntry("Gmail")).entry);
        Session thief = m_mario;
        thief.vaultKey = QByteArray(32, 'x');
        int unreadable = 0;
        QVERIFY(VaultService::list(thief, &unreadable).isEmpty());
        QCOMPARE(unreadable, 1);
    }

    void userCreatedBeforeVaultGetsSaltOnLogin()
    {
        QSqlQuery q("UPDATE users SET vault_salt = NULL WHERE username = 'mario'");
        const auto first = AuthService::login("mario", "segreto1");
        QVERIFY(first.user && !first.user->vaultSalt.isEmpty());
        QCOMPARE(AuthService::login("mario", "segreto1").vaultKey, first.vaultKey);
    }

    void generatedPasswords()
    {
        const QString a = VaultService::generatePassword(20, true);
        QCOMPARE(a.size(), 20);
        QVERIFY(a != VaultService::generatePassword(20, true));
        QVERIFY(std::any_of(a.cbegin(), a.cend(), [](QChar c) { return c.isLower(); }));
        QVERIFY(std::any_of(a.cbegin(), a.cend(), [](QChar c) { return c.isUpper(); }));
        QVERIFY(std::any_of(a.cbegin(), a.cend(), [](QChar c) { return c.isDigit(); }));
        QVERIFY(std::any_of(a.cbegin(), a.cend(), [](QChar c) { return !c.isLetterOrNumber(); }));

        const QString b = VaultService::generatePassword(12, false);
        QCOMPARE(b.size(), 12);
        QVERIFY(std::all_of(b.cbegin(), b.cend(), [](QChar c) { return c.isLetterOrNumber(); }));
    }

    void migratesDatabaseFromV1()
    {
        Database::close();
        QTemporaryDir dir;
        const QString path = dir.filePath("v1.db");

        // Si costruisce un DB "vecchio": schema attuale riportato a v1, con un utente senza vault_salt.
        QVERIFY(Database::open(path));
        QVERIFY(AuthService::registerUser("mario", "segreto1", "segreto1").user);
        {
            QSqlQuery q;
            QVERIFY(q.exec("DROP TABLE transaction_tags")); // v3
            QVERIFY(q.exec("DROP TABLE tags"));
            QVERIFY(q.exec("DROP TABLE vault_entries"));    // v2
            QVERIFY(q.exec("ALTER TABLE users DROP COLUMN vault_salt"));
            QVERIFY(q.exec("PRAGMA user_version = 1"));
        }
        Database::close();

        QVERIFY(Database::open(path));
        QSqlQuery q("PRAGMA user_version");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 4);
        const Session session = AuthService::login("mario", "segreto1").session();
        QCOMPARE(session.vaultKey.size(), 32);
        QVERIFY(VaultService::create(session, makeEntry("Gmail")).entry);
        QCOMPARE(VaultService::list(session).size(), 1);
        Database::close();

        QVERIFY(Database::open(":memory:")); // per cleanup()
    }

    void deletingUserDeletesVault()
    {
        QVERIFY(VaultService::create(m_mario, makeEntry("Gmail")).entry);
        QSqlQuery q;
        q.prepare("DELETE FROM users WHERE id = ?");
        q.addBindValue(m_mario.user.id);
        QVERIFY(q.exec());
        q.exec("SELECT COUNT(*) FROM vault_entries");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

private:
    Session m_mario;
};

QTEST_GUILESS_MAIN(TestVault)
#include "tst_vault.moc"
