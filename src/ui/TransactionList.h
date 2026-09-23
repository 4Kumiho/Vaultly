#pragma once

#include "core/Currency.h"
#include "core/Transaction.h"

#include <QList>
#include <QWidget>

class QVBoxLayout;

// Elenco dei movimenti raggruppati per giorno ("Oggi", "Ieri", "Lunedì 21 settembre"...).
class TransactionList : public QWidget
{
    Q_OBJECT

public:
    explicit TransactionList(QWidget *parent = nullptr);

    // `transactions` dal più recente al più vecchio.
    void setTransactions(const QList<Transaction> &transactions, const Currency &currency);

signals:
    void transactionClicked(const Transaction &transaction);

private:
    QVBoxLayout *m_layout;
};
