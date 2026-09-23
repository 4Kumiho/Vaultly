#include "core/BalanceHistory.h"

#include <algorithm>

namespace {

QList<Transaction> sortedByTime(QList<Transaction> transactions)
{
    std::stable_sort(transactions.begin(), transactions.end(),
                     [](const Transaction &a, const Transaction &b) { return a.occurredAt < b.occurredAt; });
    return transactions;
}

} // namespace

qint64 BalanceHistory::balanceAt(qint64 initialBalance, const QList<Transaction> &transactions,
                                 const QDateTime &time)
{
    qint64 balance = initialBalance;
    for (const Transaction &t : transactions) {
        if (t.occurredAt <= time)
            balance += t.signedAmount();
    }
    return balance;
}

QList<BalanceHistory::Point> BalanceHistory::steps(qint64 initialBalance, const QList<Transaction> &transactions,
                                                   const QDateTime &from, const QDateTime &to)
{
    QList<Point> points;
    qint64 balance = balanceAt(initialBalance, transactions, from);
    points.append({from, balance});

    for (const Transaction &t : sortedByTime(transactions)) {
        if (t.occurredAt <= from || t.occurredAt > to)
            continue;
        points.append({t.occurredAt, balance});
        balance += t.signedAmount();
        points.append({t.occurredAt, balance});
    }

    points.append({to, balance});
    return points;
}

BalanceHistory::Totals BalanceHistory::totals(const QList<Transaction> &transactions, const QDateTime &from,
                                              const QDateTime &to)
{
    Totals totals;
    for (const Transaction &t : transactions) {
        if (t.occurredAt < from || t.occurredAt > to)
            continue;
        (t.type == TransactionType::Income ? totals.income : totals.expense) += t.amount;
    }
    return totals;
}

QDateTime BalanceHistory::periodStart(Period period, const QDateTime &now, const QList<Transaction> &transactions)
{
    switch (period) {
    case Period::Day:
        return now.addDays(-1);
    case Period::Week:
        return now.addDays(-7);
    case Period::Month:
        return now.addMonths(-1);
    case Period::HalfYear:
        return now.addMonths(-6);
    case Period::Year:
        return now.addYears(-1);
    case Period::All:
        break;
    }

    if (transactions.isEmpty())
        return now.addMonths(-1);
    QDateTime earliest = transactions.first().occurredAt;
    for (const Transaction &t : transactions)
        earliest = std::min(earliest, t.occurredAt);
    return std::min(earliest, now.addDays(-1));
}
