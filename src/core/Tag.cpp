#include "core/Tag.h"

QString normalizeTagName(QString name)
{
    name = name.trimmed();
    while (name.startsWith('#'))
        name.remove(0, 1);
    return name.simplified();
}

QString toDbString(TagPeriod period)
{
    switch (period) {
    case TagPeriod::Weekly:
        return QStringLiteral("weekly");
    case TagPeriod::Monthly:
        return QStringLiteral("monthly");
    case TagPeriod::Range:
        return QStringLiteral("range");
    case TagPeriod::None:
        break;
    }
    return QStringLiteral("none");
}

TagPeriod tagPeriodFromDb(const QString &value)
{
    if (value == QLatin1String("weekly"))
        return TagPeriod::Weekly;
    if (value == QLatin1String("monthly"))
        return TagPeriod::Monthly;
    if (value == QLatin1String("range"))
        return TagPeriod::Range;
    return TagPeriod::None;
}

void Budget::window(const Tag &tag, const QDateTime &now, QDateTime *from, QDateTime *to)
{
    const QDate today = now.date();
    QDate first;
    QDate last;
    switch (tag.period) {
    case TagPeriod::None:
        *from = QDateTime();
        *to = QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59));
        return;
    case TagPeriod::Weekly:
        first = today.addDays(1 - today.dayOfWeek()); // lunedì
        last = first.addDays(6);
        break;
    case TagPeriod::Monthly:
        first = QDate(today.year(), today.month(), 1);
        last = first.addMonths(1).addDays(-1);
        break;
    case TagPeriod::Range:
        first = tag.start;
        last = tag.end;
        break;
    }
    *from = QDateTime(first, QTime(0, 0));
    *to = QDateTime(last, QTime(23, 59, 59, 999));
}

BudgetStatus::Level Budget::levelFor(qint64 spent, qint64 budget)
{
    if (budget <= 0)
        return BudgetStatus::Level::NoBudget;
    if (spent > budget)
        return BudgetStatus::Level::Over;
    if (double(spent) >= double(budget) * kWarningRatio)
        return BudgetStatus::Level::Warning;
    return BudgetStatus::Level::Ok;
}
