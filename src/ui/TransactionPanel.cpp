#include "ui/TransactionPanel.h"

#include "core/Money.h"
#include "core/TransactionService.h"
#include "db/CategoryRepository.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
// Indici dei segmenti del tipo di movimento.
constexpr int kExpense = 0;
constexpr int kIncome = 1;
} // namespace

TransactionPanel::TransactionPanel(QWidget *parent)
    : SidePanel(parent)
    , m_categories(CategoryRepository::all())
{
    QFrame *d = drawer();

    m_title = Components::label({}, "title", d);
    m_subtitle = Components::label({}, "subtitle", d);

    m_type = new SegmentedControl(d);
    m_type->addSegment(tr("Uscita"))->setProperty("tone", "expense");
    m_type->addSegment(tr("Entrata"))->setProperty("tone", "income");

    m_amount = new QLineEdit(d);
    m_amount->setProperty("emphasis", "large");

    m_category = new QComboBox(d);
    m_category->setCursor(Qt::PointingHandCursor);

    m_date = new QDateTimeEdit(d);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("dd/MM/yyyy   HH:mm");

    m_description = new QLineEdit(d);
    m_description->setPlaceholderText(tr("Facoltativa, es. \"Spesa al supermercato\""));
    m_description->setMaxLength(200);

    m_error = new ErrorLabel(d);

    m_delete = Components::button({}, "danger", d);
    auto *cancelButton = Components::button(tr("Annulla"), "secondary", d);
    auto *saveButton = Components::button(tr("Salva"), "primary", d);

    auto *layout = new QVBoxLayout(d);
    layout->setContentsMargins(32, 36, 32, 28);
    layout->setSpacing(6);
    layout->addWidget(m_title);
    layout->addWidget(m_subtitle);
    layout->addSpacing(20);
    layout->addWidget(m_type);
    layout->addSpacing(12);
    Components::addField(layout, tr("Importo"), m_amount);
    Components::addField(layout, tr("Categoria"), m_category);
    Components::addField(layout, tr("Data e ora"), m_date);
    Components::addField(layout, tr("Descrizione"), m_description);
    layout->addWidget(m_error);
    layout->addStretch();
    layout->addWidget(m_delete, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addWidget(cancelButton, 1);
    buttons->addWidget(saveButton, 1);
    layout->addLayout(buttons);

    connect(m_type, &SegmentedControl::currentChanged, this, [this](int index) {
        setType(index == kIncome ? TransactionType::Income : TransactionType::Expense);
    });
    connect(cancelButton, &QPushButton::clicked, this, [this] { dismiss(); });
    connect(saveButton, &QPushButton::clicked, this, &TransactionPanel::save);
    connect(m_delete, &QPushButton::clicked, this, &TransactionPanel::deleteClicked);
    connect(m_amount, &QLineEdit::returnPressed, this, &TransactionPanel::save);
    connect(m_description, &QLineEdit::returnPressed, this, &TransactionPanel::save);
}

void TransactionPanel::openForCreate(qint64 userId, const Account &account)
{
    m_editing = false;
    m_userId = userId;
    m_account = account;
    m_transaction = {};

    prepare(tr("Nuovo movimento"));
    setType(TransactionType::Expense);
    m_amount->clear();
    m_date->setDateTime(QDateTime::currentDateTime());
    m_description->clear();
    m_delete->hide();
    open(m_amount);
}

void TransactionPanel::openForEdit(qint64 userId, const Account &account, const Transaction &transaction)
{
    m_editing = true;
    m_userId = userId;
    m_account = account;
    m_transaction = transaction;

    prepare(tr("Modifica movimento"));
    setType(transaction.type, transaction.categoryId);
    m_amount->setText(Money::formatNumber(transaction.amount, account.currency.minorUnits, QLocale(), false));
    m_date->setDateTime(transaction.occurredAt);
    m_description->setText(transaction.description);
    m_delete->setText(tr("Elimina movimento"));
    m_delete->show();
    open(m_amount);
}

void TransactionPanel::prepare(const QString &title)
{
    m_title->setText(title);
    m_subtitle->setText(tr("Conto: %1").arg(m_account.name));
    m_amount->setPlaceholderText(Money::format(0, m_account.currency));
    m_confirmDelete = false;
    m_error->clearMessage();
}

TransactionType TransactionPanel::currentType() const
{
    return m_type->currentIndex() == kIncome ? TransactionType::Income : TransactionType::Expense;
}

void TransactionPanel::setType(TransactionType type, qint64 selectCategoryId)
{
    m_type->setCurrentIndex(type == TransactionType::Income ? kIncome : kExpense);

    m_category->clear();
    for (const Category &c : std::as_const(m_categories)) {
        if (c.type == type)
            m_category->addItem(c.name, c.id);
    }
    const int index = m_category->findData(selectCategoryId);
    m_category->setCurrentIndex(index >= 0 ? index : 0);
}

void TransactionPanel::save()
{
    const auto amount = Money::parse(m_amount->text(), m_account.currency.minorUnits);
    if (!amount || *amount <= 0) {
        m_error->showMessage(tr("Inserisci un importo maggiore di zero."));
        Animations::shake(drawer());
        m_amount->setFocus();
        return;
    }

    Transaction t = m_transaction;
    t.accountId = m_account.id;
    t.type = currentType();
    t.categoryId = m_category->currentData().toLongLong();
    t.amount = *amount;
    t.occurredAt = m_date->dateTime();
    t.description = m_description->text();

    const auto result = m_editing ? TransactionService::update(m_userId, t) : TransactionService::create(m_userId, t);
    if (!result.transaction) {
        m_error->showMessage(result.error);
        Animations::shake(drawer());
        return;
    }
    emit changed();
    dismiss();
}

void TransactionPanel::deleteClicked()
{
    // Doppio click per confermare, senza finestre di dialogo.
    if (!m_confirmDelete) {
        m_confirmDelete = true;
        m_delete->setText(tr("Clicca di nuovo per confermare"));
        return;
    }
    if (!TransactionService::remove(m_userId, m_transaction.id)) {
        m_error->showMessage(tr("Impossibile eliminare il movimento."));
        return;
    }
    emit changed();
    dismiss();
}
