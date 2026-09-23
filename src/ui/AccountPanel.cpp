#include "ui/AccountPanel.h"

#include "core/AccountService.h"
#include "core/Money.h"
#include "db/AccountRepository.h"
#include "db/CurrencyRepository.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AccountPanel::AccountPanel(QWidget *parent)
    : SidePanel(parent)
    , m_currencies(CurrencyRepository::all())
{
    QFrame *d = drawer();

    m_title = Components::label({}, "title", d);
    m_subtitle = Components::label({}, "subtitle", d);
    m_subtitle->setWordWrap(true);

    m_name = new QLineEdit(d);
    m_name->setPlaceholderText(tr("es. Conto corrente, Carta, Contanti"));

    m_currency = new QComboBox(d);
    m_currency->setCursor(Qt::PointingHandCursor);
    for (const Currency &c : std::as_const(m_currencies))
        m_currency->addItem(QString("%1  %2").arg(c.symbol, c.code), c.code);
    m_currencyHint = Components::label(tr("La valuta non si può cambiare dopo la creazione."), "muted", d);
    m_currencyHint->setWordWrap(true);

    m_balance = new QLineEdit(d);
    m_error = new ErrorLabel(d);
    m_delete = Components::button({}, "danger", d);

    auto *cancelButton = Components::button(tr("Annulla"), "secondary", d);
    auto *saveButton = Components::button(tr("Salva"), "primary", d);

    auto *layout = new QVBoxLayout(d);
    layout->setContentsMargins(32, 36, 32, 28);
    layout->setSpacing(6);
    layout->addWidget(m_title);
    layout->addWidget(m_subtitle);
    layout->addSpacing(24);
    Components::addField(layout, tr("Nome"), m_name);
    Components::addField(layout, tr("Valuta"), m_currency);
    layout->addWidget(m_currencyHint);
    layout->addSpacing(8);
    Components::addField(layout, tr("Saldo iniziale"), m_balance);
    layout->addWidget(m_error);
    layout->addStretch();
    layout->addWidget(m_delete, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addWidget(cancelButton, 1);
    buttons->addWidget(saveButton, 1);
    layout->addLayout(buttons);

    connect(m_currency, &QComboBox::currentIndexChanged, this, &AccountPanel::updateBalancePlaceholder);
    connect(cancelButton, &QPushButton::clicked, this, [this] { dismiss(); });
    connect(saveButton, &QPushButton::clicked, this, &AccountPanel::save);
    connect(m_delete, &QPushButton::clicked, this, &AccountPanel::deleteClicked);
    connect(m_name, &QLineEdit::returnPressed, this, &AccountPanel::save);
    connect(m_balance, &QLineEdit::returnPressed, this, &AccountPanel::save);
}

void AccountPanel::openForCreate(qint64 userId)
{
    m_editing = false;
    m_account = {};
    m_account.userId = userId;

    m_title->setText(tr("Nuovo conto"));
    m_subtitle->setText(tr("Scegli un nome, la valuta e quanto c'è sul conto oggi."));
    m_name->clear();
    m_balance->clear();
    m_currency->setEnabled(true);
    const int eur = m_currency->findData("EUR");
    m_currency->setCurrentIndex(eur >= 0 ? eur : 0);
    m_currencyHint->hide();
    m_error->clearMessage();
    m_delete->hide();
    updateBalancePlaceholder();
    open(m_name);
}

void AccountPanel::openForEdit(const Account &account)
{
    m_editing = true;
    m_account = account;

    m_title->setText(tr("Modifica conto"));
    m_subtitle->setText(tr("Puoi correggere il nome o il saldo iniziale: il saldo attuale si ricalcola da solo."));
    m_name->setText(account.name);
    m_currency->setCurrentIndex(m_currency->findData(account.currency.code));
    m_currency->setEnabled(false);
    m_currencyHint->show();
    m_balance->setText(Money::formatNumber(account.initialBalance, account.currency.minorUnits, QLocale(), false));
    m_error->clearMessage();
    m_confirmDelete = false;
    m_delete->setText(tr("Elimina conto"));
    m_delete->show();
    updateBalancePlaceholder();
    open(m_name);
}

Currency AccountPanel::selectedCurrency() const
{
    const QString code = m_currency->currentData().toString();
    for (const Currency &c : m_currencies) {
        if (c.code == code)
            return c;
    }
    return {};
}

void AccountPanel::updateBalancePlaceholder()
{
    m_balance->setPlaceholderText(Money::format(0, selectedCurrency()));
}

void AccountPanel::save()
{
    const Currency currency = selectedCurrency();

    // Campo vuoto = saldo iniziale zero.
    const QString balanceText = m_balance->text().trimmed();
    const auto balance = balanceText.isEmpty() ? std::optional<qint64>(0)
                                               : Money::parse(balanceText, currency.minorUnits);
    if (!balance) {
        m_error->showMessage(tr("Saldo iniziale non valido."));
        Animations::shake(drawer());
        m_balance->setFocus();
        return;
    }

    const auto result = m_editing ? AccountService::update(m_account, m_name->text(), *balance)
                                  : AccountService::create(m_account.userId, m_name->text(), currency, *balance);
    if (!result.account) {
        m_error->showMessage(result.error);
        Animations::shake(drawer());
        return;
    }
    emit saved(*result.account);
    dismiss();
}

void AccountPanel::deleteClicked()
{
    // Doppio click per confermare, senza finestre di dialogo. Il primo click avvisa
    // di quanti movimenti andranno persi.
    if (!m_confirmDelete) {
        m_confirmDelete = true;
        const int count = AccountRepository::transactionCount(m_account.id, m_account.userId);
        if (count == 1)
            m_error->showMessage(tr("Eliminando il conto perderai anche il suo movimento."));
        else if (count > 1)
            m_error->showMessage(tr("Eliminando il conto perderai anche i suoi %1 movimenti.").arg(count));
        m_delete->setText(tr("Clicca di nuovo per confermare"));
        return;
    }
    if (!AccountService::remove(m_account)) {
        m_error->showMessage(tr("Impossibile eliminare il conto."));
        Animations::shake(drawer());
        return;
    }
    emit deleted(m_account.id);
    dismiss();
}
