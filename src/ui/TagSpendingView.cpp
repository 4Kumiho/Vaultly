#include "ui/TagSpendingView.h"

#include "core/Money.h"
#include "ui/Components.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLocale>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kBarMax = 1000;

QString tr(const char *text)
{
    return QCoreApplication::translate("TagSpendingView", text);
}

QString expensesCount(int count)
{
    return count == 1 ? tr("1 spesa") : tr("%1 spese").arg(count);
}

QString shortDate(const QDate &d)
{
    return QLocale().toString(d, d.year() == QDate::currentDate().year() ? "d MMM" : "d MMM yyyy");
}

// Scadenza in breve: "Mensile · settembre", "20 set – 30 set", ...
QString periodText(const Tag &tag, const BudgetStatus &s)
{
    switch (tag.period) {
    case TagPeriod::None:
        return tr("Nessuna scadenza");
    case TagPeriod::Weekly:
        return tr("Settimanale · %1 – %2").arg(shortDate(s.from.date()), shortDate(s.to.date()));
    case TagPeriod::Monthly:
        return tr("Mensile · %1").arg(QLocale().toString(s.from.date(), "MMMM"));
    case TagPeriod::Range:
        return QString("%1 – %2").arg(shortDate(tag.start), shortDate(tag.end));
    }
    return {};
}

// Stato del tetto in parole, con il tono (colore) da usare.
QString budgetText(const TagSpendingView::Item &item, const char **tone)
{
    const BudgetStatus &s = item.status;
    const Currency &c = item.budgetCurrency;
    *tone = "";
    if (s.upcoming)
        return tr("Inizia il %1").arg(shortDate(item.tag.start));
    QString text;
    switch (s.level) {
    case BudgetStatus::Level::Over:
        *tone = "negative";
        text = tr("Tetto superato di %1").arg(Money::format(s.spent - s.budget, c));
        break;
    case BudgetStatus::Level::Warning:
        *tone = "warning";
        text = s.spent == s.budget ? tr("Tetto raggiunto")
                                   : tr("Quasi al limite · restano %1").arg(Money::format(s.budget - s.spent, c));
        break;
    default:
        text = tr("Restano %1").arg(Money::format(s.budget - s.spent, c));
        break;
    }
    if (s.finished)
        text = tr("Concluso il %1 · %2").arg(shortDate(item.tag.end), text.left(1).toLower() + text.mid(1));
    else if (item.tag.period == TagPeriod::Range) {
        const qint64 days = QDate::currentDate().daysTo(item.tag.end);
        text += days == 0 ? tr(" · ultimo giorno") : tr(" · mancano %1 giorni").arg(days);
    }
    return text;
}

class TagRow : public ClickableFrame
{
public:
    QPushButton *edit = nullptr;

    // `tag` nullptr = riga "Senza etichetta".
    TagRow(const TagSpendingView::Item *item, const TagStats::Spending &untagged, qint64 maxSpending,
           const Currency &accountCurrency, const QString &periodName, bool selected, QWidget *parent)
        : ClickableFrame(parent)
    {
        setObjectName("tagRow");
        setProperty("selected", selected);

        const bool hasBudget = item && item->tag.budget;
        const TagStats::Spending &spending = item ? item->spending : untagged;

        auto *name = item ? Components::label(item->tag.name, "tag", this)
                          : Components::label(tr("Senza etichetta"), "muted", this);
        name->setTextFormat(Qt::PlainText);
        auto *period = Components::label(item ? periodText(item->tag, item->status) : QString(), "hint", this);

        QString amountText;
        int barValue = 0;
        const char *barTone = item ? "" : "muted";
        if (hasBudget) {
            const auto &s = item->status;
            amountText = tr("%1 di %2").arg(Money::format(s.spent, item->budgetCurrency),
                                            Money::format(s.budget, item->budgetCurrency));
            barValue = int(std::min(1.0, s.ratio()) * kBarMax);
            barTone = s.level == BudgetStatus::Level::Over      ? "over"
                : s.level == BudgetStatus::Level::Warning ? "warning"
                                                          : "ok";
        } else {
            amountText = Money::format(spending.total, accountCurrency);
            barValue = maxSpending > 0 ? int(spending.total * kBarMax / maxSpending) : 0;
        }
        auto *amount = Components::label(amountText, "rowAmount", this);
        amount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto *bar = new QProgressBar(this);
        bar->setObjectName("tagBar");
        bar->setProperty("tone", barTone);
        bar->setTextVisible(false);
        bar->setRange(0, kBarMax);
        bar->setValue(0);
        bar->setFixedHeight(6);

        // Riga sotto la barra: stato del tetto, oppure numero di spese nel periodo.
        const char *statusTone = "";
        const QString statusText = hasBudget
            ? budgetText(*item, &statusTone) + tr("  ·  tutti i conti in %1").arg(item->budgetCurrency.code)
            : tr("%1 · %2, questo conto").arg(expensesCount(spending.count), periodName.toLower());
        auto *status = Components::label(statusText, hasBudget ? "budgetStatus" : "hint", this);
        status->setProperty("tone", statusTone);

        auto *top = new QHBoxLayout;
        top->setSpacing(10);
        top->addWidget(name);
        top->addWidget(period);
        top->addStretch();
        top->addWidget(amount);

        auto *bottom = new QHBoxLayout;
        bottom->setSpacing(10);
        bottom->addWidget(status, 1);
        if (item) {
            edit = Components::button(tr("Modifica"), "link", this);
            edit->setFocusPolicy(Qt::NoFocus);
            bottom->addWidget(edit);
        }

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(6);
        layout->addLayout(top);
        layout->addWidget(bar);
        layout->addLayout(bottom);

        for (QWidget *w : {static_cast<QWidget *>(name), static_cast<QWidget *>(period),
                           static_cast<QWidget *>(amount), static_cast<QWidget *>(bar),
                           static_cast<QWidget *>(status)})
            w->setAttribute(Qt::WA_TransparentForMouseEvents);

        // La barra si riempie con un'animazione (un filo sempre visibile se c'è spesa).
        const bool anything = hasBudget ? item->status.spent > 0 : spending.total > 0;
        auto *anim = new QPropertyAnimation(bar, "value", bar);
        anim->setDuration(650);
        anim->setStartValue(0);
        anim->setEndValue(anything ? std::max(barValue, 8) : 0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
};

// Prima i tetti (superati, poi quasi al limite, poi gli altri), poi le etichette senza tetto per spesa.
int urgency(const TagSpendingView::Item &item)
{
    if (!item.tag.budget)
        return 0;
    if (item.status.finished || item.status.upcoming)
        return 1;
    switch (item.status.level) {
    case BudgetStatus::Level::Over:
        return 4;
    case BudgetStatus::Level::Warning:
        return 3;
    default:
        return 2;
    }
}

} // namespace

TagSpendingView::TagSpendingView(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);
}

void TagSpendingView::setItems(const QList<Item> &items, const TagStats::Spending &untagged,
                               const Currency &accountCurrency, const QString &periodName,
                               const std::optional<QString> &selected)
{
    while (QLayoutItem *layoutItem = m_layout->takeAt(0)) {
        if (QWidget *w = layoutItem->widget())
            w->deleteLater();
        delete layoutItem;
    }

    if (items.isEmpty()) {
        auto *empty = Components::label(tr("Non hai ancora etichette. Creane una con \"+ Nuova etichetta\" "
                                           "oppure scrivila direttamente in un movimento."),
                                        "muted", this);
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(60);
        m_layout->addWidget(empty);
        return;
    }

    QList<Item> sorted = items;
    std::stable_sort(sorted.begin(), sorted.end(), [](const Item &a, const Item &b) {
        const int ua = urgency(a), ub = urgency(b);
        if (ua != ub)
            return ua > ub;
        if (a.tag.budget && b.tag.budget)
            return a.status.ratio() > b.status.ratio();
        return a.spending.total > b.spending.total;
    });

    qint64 maxSpending = untagged.total;
    for (const Item &i : sorted) {
        if (!i.tag.budget)
            maxSpending = std::max(maxSpending, i.spending.total);
    }

    for (const Item &i : std::as_const(sorted)) {
        const bool isSelected = selected && !selected->isEmpty()
            && selected->compare(i.tag.name, Qt::CaseInsensitive) == 0;
        auto *row = new TagRow(&i, untagged, maxSpending, accountCurrency, periodName, isSelected, this);
        const QString name = i.tag.name;
        const qint64 id = i.tag.id;
        connect(row, &ClickableFrame::clicked, this, [this, name] { emit tagClicked(name); });
        connect(row->edit, &QPushButton::clicked, this, [this, id] { emit editRequested(id); });
        m_layout->addWidget(row);
    }
    if (untagged.count > 0) {
        auto *row = new TagRow(nullptr, untagged, maxSpending, accountCurrency, periodName,
                               selected && selected->isEmpty(), this);
        connect(row, &ClickableFrame::clicked, this, [this] { emit tagClicked(QString("")); });
        m_layout->addWidget(row);
    }
}
