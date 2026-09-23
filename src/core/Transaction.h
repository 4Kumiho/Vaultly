#pragma once

#include <QDateTime>
#include <QString>

enum class TransactionType { Income, Expense };

inline QString toDbString(TransactionType type)
{
    return type == TransactionType::Income ? QStringLiteral("income") : QStringLiteral("expense");
}

inline TransactionType transactionTypeFromDb(const QString &value)
{
    return value == QLatin1String("income") ? TransactionType::Income : TransactionType::Expense;
}

struct Category
{
    qint64 id = 0;
    QString name;
    TransactionType type = TransactionType::Expense;
};

struct Transaction
{
    qint64 id = 0;
    qint64 accountId = 0;
    TransactionType type = TransactionType::Expense;
    qint64 categoryId = 0;
    QString categoryName; // solo in lettura, dal JOIN con categories
    qint64 amount = 0;    // in unità minime, sempre > 0: il segno lo dà `type`
    QString description;
    QDateTime occurredAt; // ora locale

    qint64 signedAmount() const { return type == TransactionType::Income ? amount : -amount; }
};
