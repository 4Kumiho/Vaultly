#include "ui/TagSpendingView.h"

#include "core/Money.h"
#include "ui/Components.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QVBoxLayout>

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

// Riga: nome e numero di spese a sinistra, totale a destra, barra sotto.
class TagRow : public ClickableFrame
{
public:
    TagRow(const TagStats::Spending &s, qint64 max, const Currency &currency, bool untagged, bool selected,
           QWidget *parent)
        : ClickableFrame(parent)
    {
        setObjectName("tagRow");
        setProperty("selected", selected);

        auto *name = untagged ? Components::label(tr("Senza etichetta"), "muted", this)
                              : Components::label(s.tag, "tag", this);
        name->setTextFormat(Qt::PlainText);
        auto *count = Components::label(expensesCount(s.count), "hint", this);
        auto *amount = Components::label(Money::format(s.total, currency), "rowAmount", this);
        amount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto *bar = new QProgressBar(this);
        bar->setObjectName("tagBar");
        bar->setProperty("tone", untagged ? "muted" : "");
        bar->setTextVisible(false);
        bar->setRange(0, kBarMax);
        bar->setValue(0);
        bar->setFixedHeight(6);

        auto *top = new QHBoxLayout;
        top->setSpacing(10);
        top->addWidget(name);
        top->addWidget(count);
        top->addStretch();
        top->addWidget(amount);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 8, 12, 10);
        layout->setSpacing(6);
        layout->addLayout(top);
        layout->addWidget(bar);

        for (QWidget *w : {static_cast<QWidget *>(name), static_cast<QWidget *>(count),
                           static_cast<QWidget *>(amount), static_cast<QWidget *>(bar)})
            w->setAttribute(Qt::WA_TransparentForMouseEvents);

        // La barra si riempie con un'animazione.
        const int target = max > 0 ? int(s.total * kBarMax / max) : 0;
        auto *anim = new QPropertyAnimation(bar, "value", bar);
        anim->setDuration(650);
        anim->setStartValue(0);
        anim->setEndValue(std::max(target, 8)); // sempre un filo visibile
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
};

} // namespace

TagSpendingView::TagSpendingView(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);
}

void TagSpendingView::setReport(const TagStats::Report &report, const Currency &currency,
                                const std::optional<QString> &selected)
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    if (report.tags.isEmpty()) {
        auto *empty = Components::label(
            report.untagged.count > 0
                ? tr("Nessuna spesa con etichetta in questo periodo. Aggiungine una dal pannello del movimento, "
                     "es. \"vacanza\" o \"auto\".")
                : tr("Nessuna spesa in questo periodo."),
            "muted", this);
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(60);
        m_layout->addWidget(empty);
        return;
    }

    qint64 max = report.untagged.total;
    for (const auto &s : report.tags)
        max = std::max(max, s.total);

    const auto addRow = [&](const TagStats::Spending &s, bool untagged) {
        const QString key = untagged ? QString("") : s.tag;
        const bool isSelected = selected && selected->compare(key, Qt::CaseInsensitive) == 0
            && selected->isEmpty() == untagged;
        auto *row = new TagRow(s, max, currency, untagged, isSelected, this);
        connect(row, &ClickableFrame::clicked, this, [this, key] { emit tagClicked(key); });
        m_layout->addWidget(row);
    };
    for (const auto &s : report.tags)
        addRow(s, false);
    if (report.untagged.count > 0)
        addRow(report.untagged, true);
}
