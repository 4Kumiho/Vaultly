#include "core/AccountService.h"
#include "core/AuthService.h"
#include "core/TransactionService.h"
#include "db/AccountRepository.h"
#include "db/CategoryRepository.h"
#include "db/Database.h"
#include "db/TransactionRepository.h"

#include <QSqlQuery>
#include <QTest>

namespace {

const Currency kEur{"EUR", "€", 2};

qint64 categoryId(const QString &name, TransactionType type)
{
    for (const Category &c : CategoryRepository::all()) {
        if (c.name == name && c.type == type)
            return c.id;
    }
    return 0;
}

Transaction makeTransaction(qint64 accountId, TransactionType type, const QString &category, qint64 amount,
                            const QDateTime &when = QDateTime(QDate(2026, 9, 1), QTime(12, 30)))
{
    Transaction t;
    t.accountId = accountId;
    t.type = type;
    t.categoryId = categoryId(category, type);
    t.amount = amount;
    t.occurredAt = when;
    return t;
}

} // namespace

class TestTransactions : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(Database::open(":memory:"));
        m_user = AuthService::registerUser("mario", "segreto1", "segreto1").user->id;
        m_account = AccountService::create(m_user, "Conto", kEur, 10000).account->id;
    }
    void cleanup() { Database::close(); }

    void categoriesPutAltroLast()
    {
        QList<QString> expense;
        for (const Category &c : CategoryRepository::all()) {
            if (c.type == TransactionType::Expense)
                expense.append(c.name);
        }
        QCOMPARE(expense.size(), 9);
        QCOMPARE(expense.first(), QString("Casa"));
        QCOMPARE(expense.last(), QString("Altro"));
    }

    void createListAndBalance()
    {
        auto r = TransactionService::create(m_user, makeTransaction(m_account, TransactionType::Income,
                                                                    "Stipendio", 150000));
        QVERIFY2(r.transaction, qPrintable(r.error));
        QCOMPARE(r.transaction->categoryName, QString("Stipendio"));
        QVERIFY(TransactionService::create(m_user, makeTransaction(m_account, TransactionType::Expense, "Spesa",
                                                                   4520, QDateTime(QDate(2026, 9, 2), QTime(9, 0))))
                    .transaction);

        const auto list = TransactionRepository::listForAccount(m_account, m_user);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].categoryName, QString("Spesa")); // più recente per primo
        QCOMPARE(list[0].occurredAt, QDateTime(QDate(2026, 9, 2), QTime(9, 0)));
        QCOMPARE(AccountRepository::currentBalance(m_account, m_user), qint64(10000 + 150000 - 4520));
    }

    void descriptionIsTrimmedAndOptional()
    {
        auto t = makeTransaction(m_account, TransactionType::Expense, "Svago", 1000);
        t.description = "  cinema  ";
        QCOMPARE(TransactionService::create(m_user, t).transaction->description, QString("cinema"));
        t.description.clear();
        QVERIFY(TransactionService::create(m_user, t).transaction);
    }

    void rejectsInvalidInput()
    {
        QVERIFY(!TransactionService::create(m_user, makeTransaction(m_account, TransactionType::Expense, "Spesa", 0))
                     .transaction);
        // Categoria di entrata su un'uscita.
        auto wrongCategory = makeTransaction(m_account, TransactionType::Expense, "Spesa", 100);
        wrongCategory.categoryId = categoryId("Stipendio", TransactionType::Income);
        QVERIFY(!TransactionService::create(m_user, wrongCategory).transaction);
        auto noDate = makeTransaction(m_account, TransactionType::Expense, "Spesa", 100);
        noDate.occurredAt = {};
        QVERIFY(!TransactionService::create(m_user, noDate).transaction);
    }

    void updateAndRemove()
    {
        auto t = *TransactionService::create(m_user, makeTransaction(m_account, TransactionType::Expense, "Spesa", 500))
                      .transaction;
        t.type = TransactionType::Income;
        t.categoryId = categoryId("Rimborsi", TransactionType::Income);
        t.amount = 700;
        QVERIFY(TransactionService::update(m_user, t).transaction);
        QCOMPARE(AccountRepository::currentBalance(m_account, m_user), qint64(10700));

        QVERIFY(TransactionService::remove(m_user, t.id));
        QCOMPARE(AccountRepository::currentBalance(m_account, m_user), qint64(10000));
        QVERIFY(TransactionRepository::listForAccount(m_account, m_user).isEmpty());
    }

    void otherUsersCannotTouchTransactions()
    {
        const qint64 anna = AuthService::registerUser("anna", "segreto1", "segreto1").user->id;
        const qint64 annaAccount = AccountService::create(anna, "Conto", kEur, 0).account->id;

        // Anna non può scrivere sul conto di Mario...
        QVERIFY(!TransactionService::create(anna, makeTransaction(m_account, TransactionType::Expense, "Spesa", 100))
                     .transaction);

        auto t = *TransactionService::create(m_user, makeTransaction(m_account, TransactionType::Expense, "Spesa", 100))
                      .transaction;
        // ...né leggere, modificare o cancellare i suoi movimenti.
        QVERIFY(TransactionRepository::listForAccount(m_account, anna).isEmpty());
        QVERIFY(!TransactionRepository::find(t.id, anna));
        t.amount = 999;
        QVERIFY(!TransactionService::update(anna, t).transaction);
        QVERIFY(!TransactionService::remove(anna, t.id));

        // E Mario non può spostare un movimento sul conto di Anna.
        t.accountId = annaAccount;
        QVERIFY(TransactionService::update(m_user, t).transaction);
        QCOMPARE(TransactionRepository::find(t.id, m_user)->accountId, m_account);
    }

    void deletingAccountDeletesTransactions()
    {
        QVERIFY(TransactionService::create(m_user, makeTransaction(m_account, TransactionType::Expense, "Spesa", 100))
                    .transaction);
        QSqlQuery q;
        q.prepare("DELETE FROM accounts WHERE id = ?");
        q.addBindValue(m_account);
        QVERIFY(q.exec());
        q.exec("SELECT COUNT(*) FROM transactions");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

private:
    qint64 m_user = 0;
    qint64 m_account = 0;
};

QTEST_GUILESS_MAIN(TestTransactions)
#include "tst_transactions.moc"
