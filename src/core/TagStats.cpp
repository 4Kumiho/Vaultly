#include "core/TagStats.h"

#include <QHash>

#include <algorithm>

TagStats::Report TagStats::expensesByTag(const QList<Transaction> &transactions, const QDateTime &from,
                                         const QDateTime &to)
{
    Report report;
    QHash<QString, qsizetype> index; // nome in minuscolo → posizione in report.tags

    for (const Transaction &t : transactions) {
        if (t.type != TransactionType::Expense || t.occurredAt < from || t.occurredAt > to)
            continue;
        if (t.tags.isEmpty()) {
            report.untagged.total += t.amount;
            ++report.untagged.count;
            continue;
        }
        for (const QString &tag : t.tags) {
            const QString key = tag.toLower();
            auto it = index.constFind(key);
            if (it == index.constEnd()) {
                it = index.insert(key, report.tags.size());
                report.tags.append({tag, 0, 0});
            }
            Spending &s = report.tags[*it];
            s.total += t.amount;
            ++s.count;
        }
    }

    std::sort(report.tags.begin(), report.tags.end(), [](const Spending &a, const Spending &b) {
        if (a.total != b.total)
            return a.total > b.total;
        return a.tag.compare(b.tag, Qt::CaseInsensitive) < 0;
    });
    return report;
}
