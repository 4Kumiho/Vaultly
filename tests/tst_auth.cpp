#include "core/AuthService.h"
#include "db/Database.h"

#include <QSqlQuery>
#include <QTest>

class TestAuth : public QObject
{
    Q_OBJECT

private slots:
    void init() { QVERIFY(Database::open(":memory:")); }
    void cleanup() { Database::close(); }

    void seedsReferenceData()
    {
        QSqlQuery q("SELECT COUNT(*) FROM currencies");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 5);

        q.exec("SELECT COUNT(*) FROM categories WHERE type = 'income'");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 5);

        q.exec("SELECT COUNT(*) FROM categories WHERE type = 'expense'");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 9);
    }

    void registerThenLogin()
    {
        const auto reg = AuthService::registerUser("  mario  ", "segreto1", "segreto1");
        QVERIFY2(reg.user, qPrintable(reg.error));
        QCOMPARE(reg.user->username, QString("mario"));
        QVERIFY(reg.user->id > 0);

        const auto login = AuthService::login("Mario", "segreto1");
        QVERIFY2(login.user, qPrintable(login.error));
        QCOMPARE(login.user->id, reg.user->id);
    }

    void passwordIsNotStoredInClear()
    {
        QVERIFY(AuthService::registerUser("mario", "segreto1", "segreto1").user);
        QSqlQuery q("SELECT password_hash FROM users");
        QVERIFY(q.next());
        QVERIFY(!q.value(0).toByteArray().contains("segreto1"));
    }

    void samePasswordGivesDifferentHashes()
    {
        const auto a = AuthService::registerUser("anna", "segreto1", "segreto1");
        const auto b = AuthService::registerUser("bruno", "segreto1", "segreto1");
        QVERIFY(a.user && b.user);
        QVERIFY(a.user->salt != b.user->salt);
        QVERIFY(a.user->passwordHash != b.user->passwordHash);
    }

    void registerRejectsMismatchedPasswords()
    {
        const auto r = AuthService::registerUser("mario", "segreto1", "segreto2");
        QVERIFY(!r.user);
        QVERIFY(!r.error.isEmpty());
    }

    void registerRejectsDuplicateUsernameIgnoringCase()
    {
        QVERIFY(AuthService::registerUser("mario", "segreto1", "segreto1").user);
        QVERIFY(!AuthService::registerUser("MARIO", "altrapass", "altrapass").user);
    }

    void registerRejectsEmptyUsername() { QVERIFY(!AuthService::registerUser("   ", "segreto1", "segreto1").user); }

    void registerRejectsShortPassword() { QVERIFY(!AuthService::registerUser("mario", "abc", "abc").user); }

    void loginRejectsWrongPassword()
    {
        QVERIFY(AuthService::registerUser("mario", "segreto1", "segreto1").user);
        QVERIFY(!AuthService::login("mario", "sbagliata").user);
    }

    void loginRejectsUnknownUser() { QVERIFY(!AuthService::login("nessuno", "segreto1").user); }
};

QTEST_GUILESS_MAIN(TestAuth)
#include "tst_auth.moc"
