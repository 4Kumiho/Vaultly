#include "core/AccountService.h"
#include "core/AuthService.h"
#include "db/AccountRepository.h"
#include "db/Database.h"

#include <QSqlQuery>
#include <QTest>

namespace {

const Currency kEur{"EUR", "€", 2};

qint64 registerUser(const QString &name)
{
    const auto r = AuthService::registerUser(name, "segreto1", "segreto1");
    return r.user ? r.user->id : 0;
}

void addTransaction(qint64 accountId, qint64 amount, const char *type)
{
    QSqlQuery q;
    q.prepare("INSERT INTO transactions (account_id, category_id, amount, type, occurred_at) "
              "VALUES (?, (SELECT id FROM categories WHERE type = ? LIMIT 1), ?, ?, '2026-01-01T00:00:00Z')");
    q.addBindValue(accountId);
    q.addBindValue(type);
    q.addBindValue(amount);
    q.addBindValue(type);
    QVERIFY(q.exec());
}

} // namespace

class TestAccounts : public QObject
{
    Q_OBJECT

private slots:
    void init() { QVERIFY(Database::open(":memory:")); }
    void cleanup() { Database::close(); }

    void createAndList()
    {
        const qint64 user = registerUser("mario");
        const auto r = AccountService::create(user, "  Conto corrente ", kEur, 150000);
        QVERIFY2(r.account, qPrintable(r.error));
        QCOMPARE(r.account->name, QString("Conto corrente"));

        const auto accounts = AccountRepository::listForUser(user);
        QCOMPARE(accounts.size(), 1);
        QCOMPARE(accounts[0].currency.code, QString("EUR"));
        QCOMPARE(accounts[0].initialBalance, qint64(150000));
    }

    void rejectsEmptyName() { QVERIFY(!AccountService::create(registerUser("mario"), "  ", kEur, 0).account); }

    void rejectsDuplicateNameIgnoringCase()
    {
        const qint64 user = registerUser("mario");
        QVERIFY(AccountService::create(user, "Carta", kEur, 0).account);
        QVERIFY(!AccountService::create(user, "CARTA", kEur, 0).account);
    }

    void sameNameAllowedForDifferentUsers()
    {
        QVERIFY(AccountService::create(registerUser("mario"), "Carta", kEur, 0).account);
        QVERIFY(AccountService::create(registerUser("anna"), "Carta", kEur, 0).account);
    }

    void balanceIncludesTransactions()
    {
        const qint64 user = registerUser("mario");
        const auto account = AccountService::create(user, "Conto", kEur, 10000).account;
        QVERIFY(account);
        addTransaction(account->id, 2500, "income");
        addTransaction(account->id, 4000, "expense");
        QCOMPARE(AccountRepository::currentBalance(account->id, user), qint64(10000 + 2500 - 4000));
    }

    void editingInitialBalanceChangesBalance()
    {
        const qint64 user = registerUser("mario");
        const auto account = AccountService::create(user, "Conto", kEur, 10000).account;
        QVERIFY(account);
        addTransaction(account->id, 1000, "expense");

        const auto updated = AccountService::update(*account, "Conto principale", 50000);
        QVERIFY2(updated.account, qPrintable(updated.error));
        QCOMPARE(AccountRepository::currentBalance(account->id, user), qint64(49000));
        QCOMPARE(AccountRepository::find(account->id, user)->name, QString("Conto principale"));
    }

    void renameToOwnNameIsAllowed()
    {
        const auto account = AccountService::create(registerUser("mario"), "Conto", kEur, 0).account;
        QVERIFY(account);
        QVERIFY(AccountService::update(*account, "conto", 0).account);
    }

    void removeDeletesAccountAndTransactions()
    {
        const qint64 user = registerUser("mario");
        const auto account = AccountService::create(user, "Conto", kEur, 10000).account;
        const auto other = AccountService::create(user, "Carta", kEur, 0).account;
        QVERIFY(account && other);
        addTransaction(account->id, 2500, "income");
        addTransaction(account->id, 4000, "expense");
        addTransaction(other->id, 100, "expense");
        QCOMPARE(AccountRepository::transactionCount(account->id, user), 2);

        QVERIFY(AccountService::remove(*account));
        QVERIFY(!AccountRepository::find(account->id, user));
        QCOMPARE(AccountRepository::listForUser(user).size(), 1);

        // Spariscono solo i movimenti del conto eliminato.
        QSqlQuery q("SELECT COUNT(*) FROM transactions");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }

    void usersCannotDeleteOthersAccounts()
    {
        const qint64 mario = registerUser("mario");
        const qint64 anna = registerUser("anna");
        const auto account = AccountService::create(mario, "Conto", kEur, 0).account;
        QVERIFY(account);
        QVERIFY(!AccountRepository::remove(account->id, anna));
        QCOMPARE(AccountRepository::transactionCount(account->id, anna), 0);
        QVERIFY(AccountRepository::find(account->id, mario));
    }

    void usersCannotTouchOthersAccounts()
    {
        const qint64 mario = registerUser("mario");
        const qint64 anna = registerUser("anna");
        const auto account = AccountService::create(mario, "Conto", kEur, 10000).account;
        QVERIFY(account);

        QVERIFY(AccountRepository::listForUser(anna).isEmpty());
        QVERIFY(!AccountRepository::find(account->id, anna));
        QVERIFY(!AccountRepository::update(account->id, anna, "Rubato", 0));
        QCOMPARE(AccountRepository::find(account->id, mario)->initialBalance, qint64(10000));
    }
};

QTEST_GUILESS_MAIN(TestAccounts)
#include "tst_accounts.moc"
