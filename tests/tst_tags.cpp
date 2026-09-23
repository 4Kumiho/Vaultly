#include "core/AccountService.h"
#include "core/AuthService.h"
#include "core/TagStats.h"
#include "core/TransactionService.h"
#include "db/CategoryRepository.h"
#include "db/Database.h"
#include "db/TransactionRepository.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

namespace {

const Currency kEur{"EUR", "€", 2};

qint64 categoryId(const QString &name, TransactionType type)
{
    for (const Category &c : CategoryRepository::all())
        if (c.name == name && c.type == type) return c.id;
    return 0;
}

Transaction expense(qint64 accountId, qint64 amount, const QStringList &tags, int day = 1)
{
    Transaction t;
    t.accountId = accountId;
    t.type = TransactionType::Expense;
    t.categoryId = categoryId("Spesa", TransactionType::Expense);
    t.amount = amount;
    t.occurredAt = QDateTime(QDate(2026, 9, day), QTime(12, 0));
    t.tags = tags;
    return t;
}

int count(const char *table)
{
    QSqlQuery q(QString("SELECT COUNT(*) FROM %1").arg(table));
    return q.next() ? q.value(0).toInt() : -1;
}

} // namespace

class TestTags : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(Database::open(":memory:"));
        m_user = AuthService::registerUser("mario", "segreto1", "segreto1").user->id;
        m_account = AccountService::create(m_user, "Conto", kEur, 0).account->id;
    }
    void cleanup() { Database::close(); }

    void savesAndLoadsTags()
    {
        const auto r = TransactionService::create(m_user, expense(m_account, 1000, {"vacanza", "Roma"}));
        QVERIFY2(r.transaction, qPrintable(r.error));
        QCOMPARE(r.transaction->tags, QStringList({"Roma", "vacanza"}));

        const auto list = TransactionRepository::listForAccount(m_account, m_user);
        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0].tags, QStringList({"Roma", "vacanza"}));
        QCOMPARE(TransactionRepository::find(list[0].id, m_user)->tags, QStringList({"Roma", "vacanza"}));
        QCOMPARE(TransactionService::tagSuggestions(m_user), QStringList({"Roma", "vacanza"}));
    }

    void normalizesTags()
    {
        const auto r =
            TransactionService::create(m_user, expense(m_account, 1000, {"  #Auto ", "auto", "", "  ", "casa  nuova"}));
        QVERIFY(r.transaction);
        QCOMPARE(r.transaction->tags, QStringList({"Auto", "casa nuova"}));
    }

    void sameTagIsReusedIgnoringCase()
    {
        QVERIFY(TransactionService::create(m_user, expense(m_account, 1000, {"Vacanza"})).transaction);
        QVERIFY(TransactionService::create(m_user, expense(m_account, 2000, {"VACANZA"})).transaction);
        QCOMPARE(count("tags"), 1);
        QCOMPARE(TransactionService::tagSuggestions(m_user), QStringList({"Vacanza"}));
    }

    void rejectsTooManyOrTooLong()
    {
        QStringList many;
        for (int i = 0; i <= TransactionService::kMaxTags; ++i)
            many << QString("e%1").arg(i);
        QVERIFY(!TransactionService::create(m_user, expense(m_account, 1000, many)).transaction);
        QVERIFY(!TransactionService::create(m_user, expense(m_account, 1000, {QString(31, 'x')})).transaction);
        QCOMPARE(count("transactions"), 0); // niente salvato a metà
    }

    void unusedTagsDisappear()
    {
        auto t = *TransactionService::create(m_user, expense(m_account, 1000, {"vecchia", "resta"})).transaction;
        QVERIFY(TransactionService::create(m_user, expense(m_account, 500, {"resta"})).transaction);

        t.tags = {"nuova"};
        QVERIFY(TransactionService::update(m_user, t).transaction);
        QCOMPARE(TransactionService::tagSuggestions(m_user), QStringList({"nuova", "resta"}));

        QVERIFY(TransactionService::remove(m_user, t.id));
        QCOMPARE(TransactionService::tagSuggestions(m_user), QStringList({"resta"}));
    }

    void deletingAccountCleansTags()
    {
        QVERIFY(TransactionService::create(m_user, expense(m_account, 1000, {"auto"})).transaction);
        QVERIFY(AccountService::remove(*AccountService::create(m_user, "x", kEur, 0).account)); // conto vuoto
        QCOMPARE(count("tags"), 1);
        Account account;
        account.id = m_account;
        account.userId = m_user;
        QVERIFY(AccountService::remove(account));
        QCOMPARE(count("tags"), 0);
        QCOMPARE(count("transaction_tags"), 0);
    }

    void tagsArePerUser()
    {
        QVERIFY(TransactionService::create(m_user, expense(m_account, 1000, {"segreta"})).transaction);
        const qint64 anna = AuthService::registerUser("anna", "segreto1", "segreto1").user->id;
        const qint64 annaAccount = AccountService::create(anna, "Conto", kEur, 0).account->id;
        QVERIFY(TransactionService::tagSuggestions(anna).isEmpty());

        // Stesso nome per Anna: etichetta separata; cancellarla non tocca quella di Mario.
        auto t = *TransactionService::create(anna, expense(annaAccount, 100, {"segreta"})).transaction;
        QCOMPARE(count("tags"), 2);
        QVERIFY(TransactionService::remove(anna, t.id));
        QCOMPARE(TransactionService::tagSuggestions(m_user), QStringList({"segreta"}));
        QVERIFY(TransactionService::tagSuggestions(anna).isEmpty());
    }

    void spendingByTag()
    {
        QList<Transaction> txs;
        txs << expense(m_account, 1000, {"vacanza", "Roma"}, 5) << expense(m_account, 3000, {"Vacanza"}, 6)
            << expense(m_account, 700, {}, 7) << expense(m_account, 9999, {"vacanza"}, 25); // fuori periodo
        Transaction income = expense(m_account, 5000, {"vacanza"}, 6);
        income.type = TransactionType::Income;
        txs << income;

        const auto report = TagStats::expensesByTag(txs, QDateTime(QDate(2026, 9, 1), QTime(0, 0)),
                                                    QDateTime(QDate(2026, 9, 20), QTime(0, 0)));
        QCOMPARE(report.tags.size(), 2);
        QCOMPARE(report.tags[0].tag, QString("vacanza"));
        QCOMPARE(report.tags[0].total, qint64(4000)); // "vacanza" e "Vacanza" sono la stessa
        QCOMPARE(report.tags[0].count, 2);
        QCOMPARE(report.tags[1].tag, QString("Roma"));
        QCOMPARE(report.tags[1].total, qint64(1000));
        QCOMPARE(report.untagged.total, qint64(700));
        QCOMPARE(report.untagged.count, 1);
    }

    void migratesFromV2()
    {
        Database::close();
        QTemporaryDir dir;
        const QString path = dir.filePath("v2.db");
        QVERIFY(Database::open(path));
        const qint64 user = AuthService::registerUser("mario", "segreto1", "segreto1").user->id;
        const qint64 account = AccountService::create(user, "Conto", kEur, 0).account->id;
        {
            QSqlQuery q;
            QVERIFY(q.exec("DROP TABLE transaction_tags"));
            QVERIFY(q.exec("DROP TABLE tags"));
            QVERIFY(q.exec("PRAGMA user_version = 2"));
        }
        Database::close();

        QVERIFY(Database::open(path));
        QSqlQuery q("PRAGMA user_version");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 3);
        QVERIFY(TransactionService::create(user, expense(account, 1000, {"dopo migrazione"})).transaction);
        Database::close();
        QVERIFY(Database::open(":memory:")); // per cleanup()
    }

private:
    qint64 m_user = 0;
    qint64 m_account = 0;
};

QTEST_GUILESS_MAIN(TestTags)
#include "tst_tags.moc"
