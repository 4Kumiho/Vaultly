#pragma once

#include "core/Account.h"
#include "ui/SidePanel.h"

#include <QList>

class ErrorLabel;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

// Pannello laterale per creare un conto o modificarne nome e saldo iniziale.
class AccountPanel : public SidePanel
{
    Q_OBJECT

public:
    explicit AccountPanel(QWidget *parent);

    void openForCreate(qint64 userId);
    void openForEdit(const Account &account);

signals:
    void saved(const Account &account);
    void deleted(qint64 accountId);

private:
    void updateBalancePlaceholder();
    Currency selectedCurrency() const;
    void save();
    void deleteClicked();

    bool m_editing = false;
    bool m_confirmDelete = false;
    Account m_account;
    QList<Currency> m_currencies;

    QLabel *m_title;
    QLabel *m_subtitle;
    QLineEdit *m_name;
    QComboBox *m_currency;
    QLabel *m_currencyHint;
    QLineEdit *m_balance;
    ErrorLabel *m_error;
    QPushButton *m_delete;
};
