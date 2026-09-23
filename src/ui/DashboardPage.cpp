#include "ui/DashboardPage.h"

#include "core/Money.h"
#include "db/AccountRepository.h"
#include "db/TransactionRepository.h"
#include "ui/AccountCard.h"
#include "ui/AccountPanel.h"
#include "ui/AmountLabel.h"
#include "ui/Animations.h"
#include "ui/BalanceChart.h"
#include "ui/Components.h"
#include "ui/TransactionList.h"
#include "ui/TransactionPanel.h"

#include <QDate>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

using BalanceHistory::Period;

namespace {

struct PeriodInfo
{
    Period period;
    const char *shortName;
    const char *longName;
};

const PeriodInfo kPeriods[] = {
    {Period::Day, QT_TRANSLATE_NOOP("DashboardPage", "1G"), QT_TRANSLATE_NOOP("DashboardPage", "Ultime 24 ore")},
    {Period::Week, QT_TRANSLATE_NOOP("DashboardPage", "1S"), QT_TRANSLATE_NOOP("DashboardPage", "Ultima settimana")},
    {Period::Month, QT_TRANSLATE_NOOP("DashboardPage", "1M"), QT_TRANSLATE_NOOP("DashboardPage", "Ultimo mese")},
    {Period::HalfYear, QT_TRANSLATE_NOOP("DashboardPage", "6M"), QT_TRANSLATE_NOOP("DashboardPage", "Ultimi 6 mesi")},
    {Period::Year, QT_TRANSLATE_NOOP("DashboardPage", "1A"), QT_TRANSLATE_NOOP("DashboardPage", "Ultimo anno")},
    {Period::All, QT_TRANSLATE_NOOP("DashboardPage", "Tutto"), QT_TRANSLATE_NOOP("DashboardPage", "Da sempre")},
};

QString periodLongName(Period period)
{
    for (const auto &p : kPeriods) {
        if (p.period == period)
            return DashboardPage::tr(p.longName);
    }
    return {};
}

QFrame *divider(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setObjectName("divider");
    line->setFixedHeight(1);
    return line;
}

} // namespace

DashboardPage::DashboardPage(QWidget *overlayHost, QWidget *parent)
    : QWidget(parent)
{
    // Riga orizzontale scorrevole con le schede dei conti.
    auto *cardsHost = new QWidget;
    m_cardsLayout = new QHBoxLayout(cardsHost);
    m_cardsLayout->setContentsMargins(0, 0, 0, 10);
    m_cardsLayout->setSpacing(14);

    auto *cardsScroll = new QScrollArea;
    cardsScroll->setWidget(cardsHost);
    cardsScroll->setWidgetResizable(true);
    cardsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    cardsScroll->setFixedHeight(128);
    cardsScroll->setFrameShape(QFrame::NoFrame);

    m_accountView = buildAccountView();
    m_emptyState = buildEmptyState();

    // Tutto il contenuto scorre in verticale; i pannelli laterali restano sopra, fissi.
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(44, 16, 44, 36);
    layout->setSpacing(14);
    layout->addWidget(Components::label(tr("I TUOI CONTI"), "caption", content));
    layout->addWidget(cardsScroll);
    layout->addSpacing(6);
    layout->addWidget(m_accountView);
    layout->addWidget(m_emptyState);
    layout->addStretch();

    auto *pageScroll = new QScrollArea(this);
    pageScroll->setWidget(content);
    pageScroll->setWidgetResizable(true);
    pageScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageScroll->setFrameShape(QFrame::NoFrame);

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(pageScroll);

    m_accountPanel = new AccountPanel(overlayHost);
    m_transactionPanel = new TransactionPanel(overlayHost);

    connect(m_accountPanel, &AccountPanel::saved, this,
            [this](const Account &account) { reloadAccounts(account.id); });
    connect(m_accountPanel, &AccountPanel::deleted, this, [this] { reloadAccounts(); });
    connect(m_transactionPanel, &TransactionPanel::changed, this,
            [this] { reloadAccounts(m_selectedId, false); });
}

QWidget *DashboardPage::buildAccountView()
{
    auto *view = new QWidget;

    // Scheda del saldo, a sinistra.
    auto *balanceCard = Components::card(view);
    balanceCard->setFixedWidth(340);
    m_accountName = Components::label({}, "cardTitle", balanceCard);
    m_balance = new AmountLabel(balanceCard);
    m_balance->setProperty("role", "balance");
    m_initialBalance = Components::label({}, "muted", balanceCard);
    auto *editButton = Components::button(tr("Modifica"), "link", balanceCard);

    auto *balanceTop = new QHBoxLayout;
    balanceTop->addWidget(Components::label(tr("SALDO ATTUALE"), "caption", balanceCard), 1);
    balanceTop->addWidget(editButton);

    m_totalsCaption = Components::label({}, "caption", balanceCard);
    m_income = Components::label({}, "statAmount", balanceCard);
    m_income->setProperty("tone", "positive");
    m_expense = Components::label({}, "statAmount", balanceCard);
    m_expense->setProperty("tone", "negative");

    auto *incomeBox = new QVBoxLayout;
    incomeBox->setSpacing(2);
    incomeBox->addWidget(Components::label(tr("Entrate"), "muted", balanceCard));
    incomeBox->addWidget(m_income);
    auto *expenseBox = new QVBoxLayout;
    expenseBox->setSpacing(2);
    expenseBox->addWidget(Components::label(tr("Uscite"), "muted", balanceCard));
    expenseBox->addWidget(m_expense);
    auto *stats = new QHBoxLayout;
    stats->addLayout(incomeBox, 1);
    stats->addLayout(expenseBox, 1);

    auto *balanceLayout = new QVBoxLayout(balanceCard);
    balanceLayout->setContentsMargins(28, 24, 28, 24);
    balanceLayout->setSpacing(6);
    balanceLayout->addLayout(balanceTop);
    balanceLayout->addWidget(m_accountName);
    balanceLayout->addSpacing(6);
    balanceLayout->addWidget(m_balance);
    balanceLayout->addWidget(m_initialBalance);
    balanceLayout->addStretch();
    balanceLayout->addWidget(divider(balanceCard));
    balanceLayout->addSpacing(8);
    balanceLayout->addWidget(m_totalsCaption);
    balanceLayout->addLayout(stats);

    // Scheda del grafico, a destra.
    auto *chartCard = Components::card(view);
    auto *periods = new SegmentedControl(chartCard);
    for (const auto &p : kPeriods)
        periods->addSegment(tr(p.shortName), tr(p.longName));
    periods->setCurrentIndex(2); // 1M
    m_chart = new BalanceChart(chartCard);
    m_chart->setMinimumHeight(280);

    auto *chartTitles = new QVBoxLayout;
    chartTitles->setSpacing(2);
    chartTitles->addWidget(Components::label(tr("Andamento"), "sectionTitle", chartCard));
    chartTitles->addWidget(Components::label(
        tr("Rotella per lo zoom · trascina per spostarti · doppio click per ripristinare"), "hint", chartCard));
    auto *chartTop = new QHBoxLayout;
    chartTop->addLayout(chartTitles, 1);
    chartTop->addWidget(periods, 0, Qt::AlignTop);

    auto *chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setContentsMargins(24, 22, 24, 16);
    chartLayout->setSpacing(10);
    chartLayout->addLayout(chartTop);
    chartLayout->addWidget(m_chart, 1);

    auto *topRow = new QHBoxLayout;
    topRow->setSpacing(18);
    topRow->addWidget(balanceCard);
    topRow->addWidget(chartCard, 1);

    // Movimenti, sotto.
    auto *listCard = Components::card(view);
    m_periodLabel = Components::label({}, "muted", listCard);
    auto *addButton = Components::button(tr("+  Aggiungi movimento"), "primary", listCard);
    m_list = new TransactionList(listCard);

    auto *listTitles = new QVBoxLayout;
    listTitles->setSpacing(2);
    listTitles->addWidget(Components::label(tr("Movimenti"), "sectionTitle", listCard));
    listTitles->addWidget(m_periodLabel);
    auto *listTop = new QHBoxLayout;
    listTop->addLayout(listTitles, 1);
    listTop->addWidget(addButton, 0, Qt::AlignVCenter);

    auto *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(24, 22, 24, 18);
    listLayout->setSpacing(14);
    listLayout->addLayout(listTop);
    listLayout->addWidget(m_list);

    auto *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);
    layout->addLayout(topRow);
    layout->addWidget(listCard);

    connect(editButton, &QPushButton::clicked, this, [this] {
        if (const Account *account = findAccount(m_selectedId))
            m_accountPanel->openForEdit(*account);
    });
    connect(periods, &SegmentedControl::currentChanged, this, [this](int index) {
        m_period = kPeriods[index].period;
        refreshAccountView();
    });
    connect(addButton, &QPushButton::clicked, this, [this] {
        if (const Account *account = findAccount(m_selectedId))
            m_transactionPanel->openForCreate(m_user.id, *account);
    });
    connect(m_list, &TransactionList::transactionClicked, this, [this](const Transaction &t) {
        if (const Account *account = findAccount(m_selectedId))
            m_transactionPanel->openForEdit(m_user.id, *account, t);
    });
    return view;
}

QFrame *DashboardPage::buildEmptyState()
{
    auto *empty = Components::card();
    auto *createFirstButton = Components::button(tr("Crea il tuo primo conto"), "primary", empty);
    auto *layout = new QVBoxLayout(empty);
    layout->setContentsMargins(32, 40, 32, 40);
    layout->setSpacing(8);
    layout->addWidget(Components::label(tr("Nessun conto, per ora"), "title", empty), 0, Qt::AlignHCenter);
    layout->addWidget(Components::label(tr("Aggiungi un conto per iniziare a tenere traccia dei tuoi soldi."),
                                        "subtitle", empty),
                      0, Qt::AlignHCenter);
    layout->addSpacing(16);
    layout->addWidget(createFirstButton, 0, Qt::AlignHCenter);

    connect(createFirstButton, &QPushButton::clicked, this, [this] { m_accountPanel->openForCreate(m_user.id); });
    return empty;
}

void DashboardPage::setUser(const User &user)
{
    m_user = user;
    m_selectedId = 0;
    reloadAccounts();

    // Primo accesso: il pannello entra appena finita la transizione di pagina.
    if (m_accounts.isEmpty())
        QTimer::singleShot(500, this, [this] { m_accountPanel->openForCreate(m_user.id); });
}

void DashboardPage::dismissPanels()
{
    m_accountPanel->dismiss(false);
    m_transactionPanel->dismiss(false);
}

void DashboardPage::reloadAccounts(qint64 selectAccountId, bool animate)
{
    m_accounts = AccountRepository::listForUser(m_user.id);

    while (QLayoutItem *item = m_cardsLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
    m_cards.clear();

    int delay = 0;
    for (const Account &account : std::as_const(m_accounts)) {
        auto *card = new AccountCard(account, AccountRepository::currentBalance(account.id, m_user.id));
        connect(card, &AccountCard::clicked, this, [this, id = account.id] { selectAccount(id); });
        m_cardsLayout->addWidget(card);
        m_cards.append(card);
        if (animate)
            Animations::fadeIn(card, 320, delay);
        delay += 70;
    }
    auto *newCard = new NewAccountCard;
    connect(newCard, &NewAccountCard::clicked, this, [this] { m_accountPanel->openForCreate(m_user.id); });
    m_cardsLayout->addWidget(newCard);
    if (animate)
        Animations::fadeIn(newCard, 320, delay);
    m_cardsLayout->addStretch();

    const bool hasAccounts = !m_accounts.isEmpty();
    m_accountView->setVisible(hasAccounts);
    m_emptyState->setVisible(!hasAccounts);

    if (hasAccounts)
        selectAccount(findAccount(selectAccountId) ? selectAccountId : m_accounts.first().id);
}

void DashboardPage::selectAccount(qint64 accountId)
{
    const Account *account = findAccount(accountId);
    if (!account)
        return;
    m_selectedId = accountId;

    for (AccountCard *card : std::as_const(m_cards))
        card->setSelected(card->accountId() == accountId);

    m_transactions = TransactionRepository::listForAccount(accountId, m_user.id);
    m_accountName->setText(QString("%1  ·  %2").arg(account->name, account->currency.code));
    m_initialBalance->setText(tr("Saldo iniziale: %1").arg(Money::format(account->initialBalance, account->currency)));
    m_balance->setAmount(AccountRepository::currentBalance(account->id, m_user.id), account->currency);
    refreshAccountView();
}

void DashboardPage::refreshAccountView()
{
    const Account *account = findAccount(m_selectedId);
    if (!account)
        return;

    // Il grafico arriva fino ad adesso, o all'ultimo movimento se è nel futuro.
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime end = now;
    for (const Transaction &t : std::as_const(m_transactions))
        end = std::max(end, t.occurredAt);

    const QDateTime from = BalanceHistory::periodStart(m_period, now, m_transactions);
    const QDateTime historyStart = std::min(from, BalanceHistory::periodStart(Period::All, now, m_transactions));
    m_chart->setData(BalanceHistory::steps(account->initialBalance, m_transactions, historyStart, end),
                     account->currency, from, end);

    const QString period = periodLongName(m_period);
    const auto totals = BalanceHistory::totals(m_transactions, from, end);
    m_totalsCaption->setText(period.toUpper());
    const auto withSign = [account](const QString &sign, qint64 amount) {
        return (amount != 0 ? sign : QString()) + Money::format(amount, account->currency);
    };
    m_income->setText(withSign("+", totals.income));
    m_expense->setText(withSign(QLocale().negativeSign(), totals.expense));
    m_periodLabel->setText(period);

    QList<Transaction> visible;
    for (const Transaction &t : std::as_const(m_transactions)) {
        if (t.occurredAt >= from)
            visible.append(t);
    }
    m_list->setTransactions(visible, account->currency);
}

const Account *DashboardPage::findAccount(qint64 accountId) const
{
    for (const Account &a : m_accounts) {
        if (a.id == accountId)
            return &a;
    }
    return nullptr;
}
