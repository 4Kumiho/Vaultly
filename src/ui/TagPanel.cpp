#include "ui/TagPanel.h"

#include "core/Money.h"
#include "core/TagService.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QComboBox>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
// Indici dei segmenti della scadenza.
constexpr int kNone = 0;
constexpr int kWeekly = 1;
constexpr int kMonthly = 2;
constexpr int kRange = 3;

int indexFor(TagPeriod period)
{
    switch (period) {
    case TagPeriod::Weekly:
        return kWeekly;
    case TagPeriod::Monthly:
        return kMonthly;
    case TagPeriod::Range:
        return kRange;
    case TagPeriod::None:
        break;
    }
    return kNone;
}
} // namespace

TagPanel::TagPanel(QWidget *parent)
    : SidePanel(parent)
{
    QFrame *d = drawer();

    m_title = Components::label({}, "title", d);
    auto *subtitle = Components::label(tr("Dai una scadenza e un tetto di spesa: la app ti avvisa all'80% "
                                          "e quando lo superi."),
                                       "subtitle", d);
    subtitle->setWordWrap(true);

    m_name = new QLineEdit(d);
    m_name->setPlaceholderText(tr("es. viaggio Roma, auto, regali"));
    m_name->setMaxLength(kMaxTagNameLength + 1);

    m_period = new SegmentedControl(d);
    m_period->addSegment(tr("Nessuna"));
    m_period->addSegment(tr("Settimanale"));
    m_period->addSegment(tr("Mensile"));
    m_period->addSegment(tr("Date"));
    m_periodHint = Components::label({}, "muted", d);
    m_periodHint->setWordWrap(true);

    m_start = new QDateEdit(d);
    m_end = new QDateEdit(d);
    for (QDateEdit *e : {m_start, m_end}) {
        e->setCalendarPopup(true);
        e->setDisplayFormat("dd/MM/yyyy");
    }
    m_dates = new QWidget(d);
    auto *datesLayout = new QHBoxLayout(m_dates);
    datesLayout->setContentsMargins(0, 0, 0, 0);
    datesLayout->setSpacing(10);
    auto *fromBox = new QVBoxLayout;
    fromBox->setSpacing(4);
    fromBox->addWidget(Components::label(tr("Dal"), "fieldLabel", m_dates));
    fromBox->addWidget(m_start);
    auto *toBox = new QVBoxLayout;
    toBox->setSpacing(4);
    toBox->addWidget(Components::label(tr("Al"), "fieldLabel", m_dates));
    toBox->addWidget(m_end);
    datesLayout->addLayout(fromBox, 1);
    datesLayout->addLayout(toBox, 1);

    m_budget = new QLineEdit(d);
    m_budget->setPlaceholderText(tr("Nessun tetto"));
    m_currency = new QComboBox(d);
    m_currency->setCursor(Qt::PointingHandCursor);
    auto *budgetRow = new QHBoxLayout;
    budgetRow->setSpacing(8);
    budgetRow->addWidget(m_budget, 1);
    budgetRow->addWidget(m_currency);
    m_budgetHint = Components::label({}, "hint", d);
    m_budgetHint->setWordWrap(true);

    m_error = new ErrorLabel(d);
    m_delete = Components::button({}, "danger", d);
    auto *cancelButton = Components::button(tr("Annulla"), "secondary", d);
    auto *saveButton = Components::button(tr("Salva"), "primary", d);

    auto *layout = new QVBoxLayout(d);
    layout->setContentsMargins(32, 36, 32, 28);
    layout->setSpacing(6);
    layout->addWidget(m_title);
    layout->addWidget(subtitle);
    layout->addSpacing(20);
    Components::addField(layout, tr("Nome"), m_name);
    layout->addWidget(Components::label(tr("Scadenza"), "fieldLabel", d));
    layout->addWidget(m_period);
    layout->addWidget(m_periodHint);
    layout->addSpacing(4);
    layout->addWidget(m_dates);
    layout->addSpacing(10);
    layout->addWidget(Components::label(tr("Tetto di spesa (facoltativo)"), "fieldLabel", d));
    layout->addLayout(budgetRow);
    layout->addWidget(m_budgetHint);
    layout->addWidget(m_error);
    layout->addStretch();
    layout->addWidget(m_delete, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addWidget(cancelButton, 1);
    buttons->addWidget(saveButton, 1);
    layout->addLayout(buttons);

    connect(m_period, &SegmentedControl::currentChanged, this, &TagPanel::updatePeriodHint);
    connect(m_currency, &QComboBox::currentIndexChanged, this, &TagPanel::updatePeriodHint);
    connect(cancelButton, &QPushButton::clicked, this, [this] { dismiss(); });
    connect(saveButton, &QPushButton::clicked, this, &TagPanel::save);
    connect(m_delete, &QPushButton::clicked, this, &TagPanel::deleteClicked);
    connect(m_name, &QLineEdit::returnPressed, this, &TagPanel::save);
    connect(m_budget, &QLineEdit::returnPressed, this, &TagPanel::save);
}

void TagPanel::openForCreate(qint64 userId, const QList<Currency> &currencies)
{
    m_editing = false;
    m_userId = userId;
    m_tag = {};
    prepare(tr("Nuova etichetta"), currencies);
    m_delete->hide();
    open(m_name);
}

void TagPanel::openForEdit(qint64 userId, const QList<Currency> &currencies, const Tag &tag)
{
    m_editing = true;
    m_userId = userId;
    m_tag = tag;
    prepare(tr("Modifica etichetta"), currencies);
    m_delete->setText(tr("Elimina etichetta"));
    m_delete->show();
    open(m_name);
}

void TagPanel::prepare(const QString &title, const QList<Currency> &currencies)
{
    m_title->setText(title);
    m_currencies = currencies;
    m_currency->clear();
    for (const Currency &c : std::as_const(m_currencies))
        m_currency->addItem(QString("%1  %2").arg(c.symbol, c.code), c.code);
    const int currencyIndex = m_currency->findData(m_tag.currencyCode);
    m_currency->setCurrentIndex(currencyIndex >= 0 ? currencyIndex : 0);

    m_name->setText(m_tag.name);
    m_period->setCurrentIndex(indexFor(m_tag.period));
    const QDate today = QDate::currentDate();
    m_start->setDate(m_tag.start.isValid() ? m_tag.start : today);
    m_end->setDate(m_tag.end.isValid() ? m_tag.end : today.addDays(7));
    m_budget->setText(m_tag.budget ? Money::formatNumber(*m_tag.budget, selectedCurrency().minorUnits, QLocale(), false)
                                   : QString());
    m_confirmDelete = false;
    m_error->clearMessage();
    updatePeriodHint();
}

TagPeriod TagPanel::currentPeriod() const
{
    switch (m_period->currentIndex()) {
    case kWeekly:
        return TagPeriod::Weekly;
    case kMonthly:
        return TagPeriod::Monthly;
    case kRange:
        return TagPeriod::Range;
    default:
        return TagPeriod::None;
    }
}

void TagPanel::updatePeriodHint()
{
    const TagPeriod period = currentPeriod();
    m_dates->setVisible(period == TagPeriod::Range);
    switch (period) {
    case TagPeriod::None:
        m_periodHint->setText(tr("Il tetto vale per tutte le spese con questa etichetta, senza scadenza."));
        break;
    case TagPeriod::Weekly:
        m_periodHint->setText(tr("Il conteggio riparte ogni lunedì."));
        break;
    case TagPeriod::Monthly:
        m_periodHint->setText(tr("Il conteggio riparte il primo di ogni mese."));
        break;
    case TagPeriod::Range:
        m_periodHint->setText(tr("Contano le spese tra queste due date, comprese (es. un viaggio)."));
        break;
    }
    const Currency c = selectedCurrency();
    m_budget->setPlaceholderText(tr("Nessun tetto, es. %1").arg(Money::formatNumber(80000, c.minorUnits, QLocale(), false)));
    m_budgetHint->setText(tr("Conta le uscite con questa etichetta su tutti i tuoi conti in %1.").arg(c.code));
}

Currency TagPanel::selectedCurrency() const
{
    const QString code = m_currency->currentData().toString();
    for (const Currency &c : m_currencies) {
        if (c.code == code)
            return c;
    }
    return m_currencies.value(0, Currency{"EUR", "€", 2});
}

void TagPanel::save()
{
    Tag t = m_tag;
    t.name = m_name->text();
    t.period = currentPeriod();
    t.start = m_start->date();
    t.end = m_end->date();

    const QString budgetText = m_budget->text().trimmed();
    if (budgetText.isEmpty()) {
        t.budget.reset();
    } else {
        const Currency c = selectedCurrency();
        const auto amount = Money::parse(budgetText, c.minorUnits);
        if (!amount || *amount <= 0) {
            m_error->showMessage(tr("Il tetto di spesa deve essere un importo maggiore di zero."));
            Animations::shake(drawer());
            m_budget->setFocus();
            return;
        }
        t.budget = *amount;
        t.currencyCode = c.code;
    }

    const auto result = m_editing ? TagService::update(m_userId, t) : TagService::create(m_userId, t);
    if (!result.tag) {
        m_error->showMessage(result.error);
        Animations::shake(drawer());
        return;
    }
    emit changed();
    dismiss();
}

void TagPanel::deleteClicked()
{
    // Doppio click per confermare; il primo avvisa da quanti movimenti verrà tolta.
    if (!m_confirmDelete) {
        m_confirmDelete = true;
        if (m_tag.usage == 1)
            m_error->showMessage(tr("Verrà tolta da 1 movimento (il movimento resta)."));
        else if (m_tag.usage > 1)
            m_error->showMessage(tr("Verrà tolta da %1 movimenti (i movimenti restano).").arg(m_tag.usage));
        m_delete->setText(tr("Clicca di nuovo per confermare"));
        return;
    }
    if (!TagService::remove(m_userId, m_tag.id)) {
        m_error->showMessage(tr("Impossibile eliminare l'etichetta."));
        return;
    }
    emit changed();
    dismiss();
}
