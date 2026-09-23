#include "core/AccountService.h"
#include "core/AuthService.h"
#include "core/TagService.h"
#include "core/TransactionService.h"
#include "db/CategoryRepository.h"
#include "db/Database.h"
#include "db/TransactionRepository.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

// Etichette con scadenza e tetto di spesa.
namespace {

const Currency kEur{"EUR", "€", 2};
const Currency kUsd{"USD", "$", 2};

// Mercoledì 23 settembre 2026, pomeriggio.
const QDateTime kNow(QDate(2026, 9, 23), QTime(15, 0));

Transaction expense(qint64 accountId, qint64 amount, const QDateTime &when, const QStringList &tags)
{
    Transaction t;
    t.accountId = accountId;
    t.type = TransactionType::Expense;
    for (const Category &c : CategoryRepository::all())
        if (c.name == "Svago" && c.type == TransactionType::Expense) t.categoryId = c.id;
    t.amount = amount;
    t.occurredAt = when;
    t.tags = tags;
    return t;
}

Tag makeTag(const QString &name, TagPeriod period, std::optional<qint64> budget = std::nullopt,
            const QString &currency = "EUR")
{
    Tag t;
    t.name = name;
    t.period = period;
    t.budget = budget;
    t.currencyCode = currency;
    return t;
}

} // namespace

class TestBudget : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(Database::open(":memory:"));
        m_user = AuthService::registerUser("mario", "segreto1", "segreto1").user->id;
        m_eur = AccountService::create(m_user, "Conto", kEur, 0).account->id;
    }
    void cleanup() { Database::close(); }

    void windows()
    {
        QDateTime from, to;
        Budget::window(makeTag("s", TagPeriod::Weekly), kNow, &from, &to);
        QCOMPARE(from, QDateTime(QDate(2026, 9, 21), QTime(0, 0))); // lunedì
        QCOMPARE(to.date(), QDate(2026, 9, 27));                     // domenica

        Budget::window(makeTag("m", TagPeriod::Monthly), kNow, &from, &to);
        QCOMPARE(from, QDateTime(QDate(2026, 9, 1), QTime(0, 0)));
        QCOMPARE(to.date(), QDate(2026, 9, 30));

        Tag trip = makeTag("r", TagPeriod::Range);
        trip.start = QDate(2026, 9, 20);
        trip.end = QDate(2026, 9, 30);
        Budget::window(trip, kNow, &from, &to);
        QCOMPARE(from, QDateTime(QDate(2026, 9, 20), QTime(0, 0)));
        QCOMPARE(to.date(), QDate(2026, 9, 30));
        QCOMPARE(to.time().hour(), 23); // l'ultimo giorno conta tutto

        Budget::window(makeTag("n", TagPeriod::None), kNow, &from, &to);
        QVERIFY(!from.isValid());
    }

    void levels()
    {
        QCOMPARE(Budget::levelFor(799, 1000), BudgetStatus::Level::Ok);
        QCOMPARE(Budget::levelFor(800, 1000), BudgetStatus::Level::Warning);
        QCOMPARE(Budget::levelFor(1000, 1000), BudgetStatus::Level::Warning); // raggiunto, non superato
        QCOMPARE(Budget::levelFor(1001, 1000), BudgetStatus::Level::Over);
        QCOMPARE(Budget::levelFor(5, 0), BudgetStatus::Level::NoBudget);
    }

    void tripBudgetCountsOnlyItsDatesAndCurrency()
    {
        Tag trip = makeTag("viaggio Roma", TagPeriod::Range, 80000);
        trip.start = QDate(2026, 9, 20);
        trip.end = QDate(2026, 9, 30);
        const auto created = TagService::create(m_user, trip);
        QVERIFY2(created.tag, qPrintable(created.error));

        const qint64 usd = AccountService::create(m_user, "Dollari", kUsd, 0).account->id;
        const qint64 eur2 = AccountService::create(m_user, "Carta", kEur, 0).account->id;
        const QStringList tag{"viaggio Roma"};
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 30000, QDateTime(QDate(2026, 9, 20), QTime(8, 0)), tag)).transaction);
        QVERIFY(TransactionService::create(m_user, expense(eur2, 40000, QDateTime(QDate(2026, 9, 22), QTime(20, 0)), tag)).transaction);
        // Fuori periodo, in un'altra valuta, o un'entrata: non contano.
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 99999, QDateTime(QDate(2026, 9, 19), QTime(23, 0)), tag)).transaction);
        QVERIFY(TransactionService::create(m_user, expense(usd, 99999, QDateTime(QDate(2026, 9, 21), QTime(10, 0)), tag)).transaction);

        auto s = TagService::status(m_user, *created.tag, kNow);
        QCOMPARE(s.spent, qint64(70000));
        QCOMPARE(s.level, BudgetStatus::Level::Warning); // 87,5%
        QVERIFY(!s.finished && !s.upcoming);

        QVERIFY(TransactionService::create(m_user, expense(m_eur, 15000, QDateTime(QDate(2026, 9, 30), QTime(22, 0)), tag)).transaction);
        s = TagService::status(m_user, *created.tag, kNow);
        QCOMPARE(s.spent, qint64(85000));
        QCOMPARE(s.level, BudgetStatus::Level::Over);

        QVERIFY(TagService::status(m_user, *created.tag, QDateTime(QDate(2026, 10, 1), QTime(9, 0))).finished);
        QVERIFY(TagService::status(m_user, *created.tag, QDateTime(QDate(2026, 9, 10), QTime(9, 0))).upcoming);
    }

    void monthlyBudgetRestartsEachMonth()
    {
        const auto tag = TagService::create(m_user, makeTag("auto", TagPeriod::Monthly, 20000)).tag;
        QVERIFY(tag);
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 25000, QDateTime(QDate(2026, 8, 30), QTime(9, 0)), {"auto"})).transaction);
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 5000, QDateTime(QDate(2026, 9, 2), QTime(9, 0)), {"auto"})).transaction);
        const auto s = TagService::status(m_user, *tag, kNow);
        QCOMPARE(s.spent, qint64(5000)); // agosto non conta a settembre
        QCOMPARE(s.level, BudgetStatus::Level::Ok);
    }

    void tagWithoutBudget()
    {
        const auto tag = TagService::create(m_user, makeTag("regali", TagPeriod::None)).tag;
        QVERIFY(tag);
        QVERIFY(tag->currencyCode.isEmpty()); // valuta azzerata se non c'è tetto
        QCOMPARE(TagService::status(m_user, *tag, kNow).level, BudgetStatus::Level::NoBudget);
    }

    void validation()
    {
        QVERIFY(!TagService::create(m_user, makeTag("   ", TagPeriod::None)).tag);
        QVERIFY(!TagService::create(m_user, makeTag(QString(31, 'x'), TagPeriod::None)).tag);
        QVERIFY(!TagService::create(m_user, makeTag("zero", TagPeriod::Monthly, 0)).tag);
        QVERIFY(!TagService::create(m_user, makeTag("valuta", TagPeriod::Monthly, 100, "XYZ")).tag);

        Tag noDates = makeTag("senza date", TagPeriod::Range, 100);
        QVERIFY(!TagService::create(m_user, noDates).tag);
        Tag backwards = noDates;
        backwards.start = QDate(2026, 9, 30);
        backwards.end = QDate(2026, 9, 20);
        QVERIFY(!TagService::create(m_user, backwards).tag);

        QVERIFY(TagService::create(m_user, makeTag("#Casa", TagPeriod::None)).tag);
        QVERIFY(!TagService::create(m_user, makeTag("casa", TagPeriod::None)).tag); // doppione

        // Le date si azzerano se la scadenza non è "date specifiche".
        Tag weekly = makeTag("settimana", TagPeriod::Weekly, 100);
        weekly.start = QDate(2026, 1, 1);
        weekly.end = QDate(2026, 1, 2);
        const auto saved = TagService::create(m_user, weekly).tag;
        QVERIFY(saved && !saved->start.isValid() && !saved->end.isValid());
    }

    void updateAndRemove()
    {
        auto tag = *TagService::create(m_user, makeTag("auto", TagPeriod::None)).tag;
        auto tx = *TransactionService::create(m_user, expense(m_eur, 1000, kNow, {"auto", "altro"})).transaction;

        tag.name = "macchina";
        tag.period = TagPeriod::Monthly;
        tag.budget = 50000;
        tag.currencyCode = "EUR";
        QVERIFY(TagService::update(m_user, tag).tag);
        QCOMPARE(TransactionRepository::find(tx.id, m_user)->tags, QStringList({"altro", "macchina"}));

        QVERIFY(TagService::remove(m_user, tag.id));
        QCOMPARE(TransactionRepository::find(tx.id, m_user)->tags, QStringList({"altro"}));
        QCOMPARE(TransactionRepository::find(tx.id, m_user)->amount, qint64(1000)); // il movimento resta
    }

    void listIncludesUsage()
    {
        QVERIFY(TagService::create(m_user, makeTag("vuota", TagPeriod::None)).tag);
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 1000, kNow, {"usata"})).transaction);
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 1000, kNow, {"usata"})).transaction);
        const auto tags = TagService::list(m_user);
        QCOMPARE(tags.size(), 2);
        QCOMPARE(tags[0].name, QString("usata"));
        QCOMPARE(tags[0].usage, 2);
        QCOMPARE(tags[1].name, QString("vuota"));
        QCOMPARE(tags[1].usage, 0);
    }

    void usersCannotTouchOthersTags()
    {
        const auto tag = *TagService::create(m_user, makeTag("mia", TagPeriod::Monthly, 1000)).tag;
        const qint64 anna = AuthService::registerUser("anna", "segreto1", "segreto1").user->id;
        QVERIFY(TagService::list(anna).isEmpty());
        Tag stolen = tag;
        stolen.budget = 1;
        QVERIFY(!TagService::update(anna, stolen).tag);
        QVERIFY(!TagService::remove(anna, tag.id));
        QCOMPARE(TagService::list(m_user).first().budget.value_or(0), qint64(1000));
        // E il suo "mia" non conta le spese di Mario.
        QVERIFY(TransactionService::create(m_user, expense(m_eur, 900, kNow, {"mia"})).transaction);
        QCOMPARE(TagService::status(anna, tag, kNow).spent, qint64(0));
    }

    void migratesFromV3()
    {
        Database::close();
        QTemporaryDir dir;
        const QString path = dir.filePath("v3.db");
        QVERIFY(Database::open(path));
        const qint64 user = AuthService::registerUser("mario", "segreto1", "segreto1").user->id;
        const qint64 account = AccountService::create(user, "Conto", kEur, 0).account->id;
        QVERIFY(TransactionService::create(user, expense(account, 1000, kNow, {"vecchia"})).transaction);
        {
            // Riporta la tabella tags alla forma v3.
            QSqlQuery q;
            QVERIFY(q.exec("PRAGMA foreign_keys = OFF"));
            QVERIFY(q.exec("CREATE TABLE tags_v3 (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL "
                           "REFERENCES users(id) ON DELETE CASCADE, name TEXT NOT NULL COLLATE NOCASE, "
                           "UNIQUE (user_id, name))"));
            QVERIFY(q.exec("INSERT INTO tags_v3 SELECT id, user_id, name FROM tags"));
            QVERIFY(q.exec("DROP TABLE tags"));
            QVERIFY(q.exec("ALTER TABLE tags_v3 RENAME TO tags"));
            QVERIFY(q.exec("PRAGMA user_version = 3"));
        }
        Database::close();

        QVERIFY(Database::open(path));
        const auto tags = TagService::list(user);
        QCOMPARE(tags.size(), 1);
        QCOMPARE(tags[0].period, TagPeriod::None);
        QVERIFY(!tags[0].budget);
        Tag updated = tags[0];
        updated.period = TagPeriod::Weekly;
        updated.budget = 5000;
        updated.currencyCode = "EUR";
        QVERIFY(TagService::update(user, updated).tag);
        Database::close();
        QVERIFY(Database::open(":memory:")); // per cleanup()
    }

private:
    qint64 m_user = 0;
    qint64 m_eur = 0;
};

QTEST_GUILESS_MAIN(TestBudget)
#include "tst_budget.moc"
