#include "core/BalanceHistory.h"

#include <QTest>

using namespace BalanceHistory;

namespace {

QDateTime at(int day, int hour = 12)
{
    return QDateTime(QDate(2026, 9, day), QTime(hour, 0));
}

Transaction tx(TransactionType type, qint64 amount, const QDateTime &when)
{
    Transaction t;
    t.type = type;
    t.amount = amount;
    t.occurredAt = when;
    return t;
}

// Volutamente non in ordine di data.
const QList<Transaction> kTransactions = {
    tx(TransactionType::Expense, 300, at(10)),
    tx(TransactionType::Income, 1000, at(5)),
    tx(TransactionType::Expense, 200, at(20)),
};

} // namespace

class TestBalance : public QObject
{
    Q_OBJECT

private slots:
    void balanceAtIncludesOnlyPast()
    {
        QCOMPARE(balanceAt(500, kTransactions, at(1)), qint64(500));
        QCOMPARE(balanceAt(500, kTransactions, at(5)), qint64(1500)); // istante esatto incluso
        QCOMPARE(balanceAt(500, kTransactions, at(15)), qint64(1200));
        QCOMPARE(balanceAt(500, kTransactions, at(30)), qint64(1000));
    }

    void stepsCoverRangeInOrder()
    {
        const auto points = steps(500, kTransactions, at(8), at(25));
        // inizio + 2 punti per ciascuno dei due movimenti nel range + fine
        QCOMPARE(points.size(), 6);
        QCOMPARE(points.first().time, at(8));
        QCOMPARE(points.first().balance, qint64(1500));
        QCOMPARE(points[1].balance, qint64(1500));
        QCOMPARE(points[2].balance, qint64(1200));
        QCOMPARE(points[3].balance, qint64(1200));
        QCOMPARE(points[4].balance, qint64(1000));
        QCOMPARE(points.last().time, at(25));
        QCOMPARE(points.last().balance, qint64(1000));
        for (qsizetype i = 1; i < points.size(); ++i)
            QVERIFY(points[i - 1].time <= points[i].time);
    }

    void stepsWithoutTransactionsIsFlat()
    {
        const auto points = steps(500, {}, at(1), at(2));
        QCOMPARE(points.size(), 2);
        QCOMPARE(points[0].balance, qint64(500));
        QCOMPARE(points[1].balance, qint64(500));
    }

    void totalsInRange()
    {
        const auto t = totals(kTransactions, at(1), at(15));
        QCOMPARE(t.income, qint64(1000));
        QCOMPARE(t.expense, qint64(300));
    }

    void periodStarts()
    {
        const QDateTime now = at(30);
        QCOMPARE(periodStart(Period::Day, now, kTransactions), at(29));
        QCOMPARE(periodStart(Period::Week, now, kTransactions), at(23));
        QCOMPARE(periodStart(Period::Month, now, kTransactions), QDateTime(QDate(2026, 8, 30), QTime(12, 0)));
        QCOMPARE(periodStart(Period::All, now, kTransactions), at(5));
        QCOMPARE(periodStart(Period::All, now, {}), QDateTime(QDate(2026, 8, 30), QTime(12, 0)));
    }
};

QTEST_GUILESS_MAIN(TestBalance)
#include "tst_balance.moc"
