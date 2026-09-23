#pragma once

#include "core/Account.h"
#include "core/BalanceHistory.h"
#include "core/Transaction.h"
#include "core/User.h"

#include <QList>
#include <QWidget>

#include <optional>

class AccountCard;
class AccountPanel;
class AmountLabel;
class BalanceChart;
class QFrame;
class QHBoxLayout;
class QLabel;
class QPushButton;
class TagSpendingView;
class TransactionList;
class TransactionPanel;

// Sezione "Conti": schede dei conti, saldo, grafico e movimenti.
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    // I pannelli laterali coprono `overlayHost` (tutta la finestra, intestazione compresa).
    explicit DashboardPage(QWidget *overlayHost, QWidget *parent = nullptr);

    // Carica i conti dell'utente. Se non ne ha, apre subito il pannello di creazione.
    void setUser(const User &user);
    void dismissPanels();

private:
    QWidget *buildAccountView();
    QFrame *buildEmptyState();

    // Ricarica i conti dal DB e seleziona `selectAccountId` (o il primo se non esiste).
    // `animate`: fa ricomparire le schede in cascata (solo quando cambia l'elenco dei conti).
    void reloadAccounts(qint64 selectAccountId = 0, bool animate = true);
    void selectAccount(qint64 accountId);
    // Aggiorna grafico, totali e lista per il conto e il periodo correnti.
    void refreshAccountView();
    const Account *findAccount(qint64 accountId) const;

    User m_user;
    QList<Account> m_accounts;
    QList<AccountCard *> m_cards;
    qint64 m_selectedId = 0;
    QList<Transaction> m_transactions; // del conto selezionato, dal più recente
    BalanceHistory::Period m_period = BalanceHistory::Period::Month;

    QHBoxLayout *m_cardsLayout;
    QWidget *m_accountView;
    QFrame *m_emptyState;

    QLabel *m_accountName;
    AmountLabel *m_balance;
    QLabel *m_initialBalance;
    QLabel *m_income;
    QLabel *m_expense;
    QLabel *m_totalsCaption;
    BalanceChart *m_chart;
    QLabel *m_tagPeriodLabel;
    TagSpendingView *m_tagView;
    std::optional<QString> m_tagFilter; // etichetta filtrata nella lista; "" = spese senza etichetta
    QLabel *m_periodLabel;
    QPushButton *m_clearFilter;
    TransactionList *m_list;

    AccountPanel *m_accountPanel;
    TransactionPanel *m_transactionPanel;
};
