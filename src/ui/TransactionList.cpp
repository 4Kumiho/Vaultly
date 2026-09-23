#include "ui/TransactionList.h"

#include "core/Money.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QHBoxLayout>
#include <QLocale>
#include <QVBoxLayout>

namespace {

// Oltre questo numero di righe niente animazione di entrata (sarebbe lenta).
constexpr int kAnimatedRows = 25;

QString dayTitle(const QDate &day)
{
    const QDate today = QDate::currentDate();
    if (day == today)
        return QObject::tr("Oggi");
    if (day == today.addDays(-1))
        return QObject::tr("Ieri");
    return QLocale().toString(day, day.year() == today.year() ? "dddd d MMMM" : "dddd d MMMM yyyy");
}

// Riga di un movimento: iniziale della categoria, categoria e descrizione, importo e ora.
class TransactionRow : public ClickableFrame
{
public:
    TransactionRow(const Transaction &t, const Currency &currency, QWidget *parent)
        : ClickableFrame(parent)
    {
        setObjectName("listRow");
        const char *tone = t.type == TransactionType::Income ? "positive" : "negative";

        auto *icon = new QLabel(t.categoryName.left(1).toUpper(), this);
        icon->setObjectName("rowIcon");
        icon->setProperty("tone", tone);
        icon->setFixedSize(38, 38);
        icon->setAlignment(Qt::AlignCenter);

        auto *title = Components::label(t.categoryName, "rowTitle", this);
        auto *subtitle = Components::label(t.description.isEmpty() ? QString("—") : t.description, "muted", this);
        subtitle->setTextFormat(Qt::PlainText);

        const QString sign = t.type == TransactionType::Income ? "+" : QLocale().negativeSign();
        auto *amount = Components::label(sign + Money::format(t.amount, currency), "rowAmount", this);
        amount->setProperty("tone", tone);
        amount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto *time = Components::label(QLocale().toString(t.occurredAt.time(), "HH:mm"), "muted", this);
        time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto *texts = new QVBoxLayout;
        texts->setSpacing(1);
        texts->addWidget(title);
        texts->addWidget(subtitle);

        auto *right = new QVBoxLayout;
        right->setSpacing(1);
        right->addWidget(amount);
        right->addWidget(time);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 8, 14, 8);
        layout->setSpacing(14);
        layout->addWidget(icon);
        layout->addLayout(texts, 1);
        layout->addLayout(right);

        for (QLabel *l : {icon, title, subtitle, amount, time})
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
};

} // namespace

TransactionList::TransactionList(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(2);
}

void TransactionList::setTransactions(const QList<Transaction> &transactions, const Currency &currency)
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    if (transactions.isEmpty()) {
        auto *empty = Components::label(tr("Nessun movimento in questo periodo."), "muted", this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(80);
        m_layout->addWidget(empty);
        return;
    }

    QDate currentDay;
    int index = 0;
    for (const Transaction &t : transactions) {
        const QDate day = t.occurredAt.date();
        if (day != currentDay) {
            currentDay = day;
            auto *header = Components::label(dayTitle(day).toUpper(), "caption", this);
            header->setContentsMargins(4, index == 0 ? 0 : 14, 0, 4);
            m_layout->addWidget(header);
        }

        auto *row = new TransactionRow(t, currency, this);
        connect(row, &ClickableFrame::clicked, this, [this, t] { emit transactionClicked(t); });
        m_layout->addWidget(row);
        if (index < kAnimatedRows)
            Animations::fadeIn(row, 260, index * 30);
        ++index;
    }
}
