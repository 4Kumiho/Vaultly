#include "ui/AccountCard.h"

#include "core/Money.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {
constexpr QSize kCardSize(220, 108);
} // namespace

AccountCard::AccountCard(const Account &account, qint64 balance, QWidget *parent)
    : ClickableFrame(parent)
    , m_accountId(account.id)
{
    setObjectName("accountCard");
    setFixedSize(kCardSize);

    auto *name = Components::label(account.name, "cardTitle", this);
    auto *currency = Components::label(account.currency.code, "caption", this);
    auto *amount = Components::label(Money::format(balance, account.currency), "cardAmount", this);
    if (balance < 0)
        amount->setProperty("tone", "negative");

    auto *top = new QHBoxLayout;
    top->addWidget(name, 1);
    top->addWidget(currency);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->addLayout(top);
    layout->addStretch();
    layout->addWidget(amount);

    for (QLabel *l : {name, currency, amount})
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
}

void AccountCard::setSelected(bool selected)
{
    setProperty("selected", selected);
    Components::repolish(this);
}

NewAccountCard::NewAccountCard(QWidget *parent)
    : ClickableFrame(parent)
{
    setObjectName("newAccountCard");
    setFixedSize(kCardSize);

    auto *text = new QLabel(tr("+  Nuovo conto"), this);
    text->setAlignment(Qt::AlignCenter);
    text->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(text);
}
