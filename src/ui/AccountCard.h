#pragma once

#include "core/Account.h"
#include "ui/Components.h"

// Scheda di un conto nella riga in alto della dashboard: nome, valuta e saldo.
class AccountCard : public ClickableFrame
{
    Q_OBJECT

public:
    AccountCard(const Account &account, qint64 balance, QWidget *parent = nullptr);

    qint64 accountId() const { return m_accountId; }
    void setSelected(bool selected);

private:
    qint64 m_accountId;
};

// Scheda tratteggiata "+ Nuovo conto" in fondo alla riga.
class NewAccountCard : public ClickableFrame
{
    Q_OBJECT

public:
    explicit NewAccountCard(QWidget *parent = nullptr);
};
