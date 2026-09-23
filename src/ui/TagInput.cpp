#include "ui/TagInput.h"

#include "core/TransactionService.h"
#include "ui/FlowLayout.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QStringListModel>
#include <QTimer>
#include <QToolButton>

namespace {

// Chip di un'etichetta con la × per toglierla.
QFrame *makeChip(const QString &tag, QWidget *parent, const std::function<void()> &onRemove)
{
    auto *chip = new QFrame(parent);
    chip->setObjectName("tagChip");
    auto *text = new QLabel(tag, chip);
    text->setTextFormat(Qt::PlainText);
    auto *remove = new QToolButton(chip);
    remove->setText(QStringLiteral("×"));
    remove->setCursor(Qt::PointingHandCursor);
    remove->setToolTip(QCoreApplication::translate("TagInput", "Togli etichetta"));
    remove->setFocusPolicy(Qt::NoFocus);
    QObject::connect(remove, &QToolButton::clicked, chip, onRemove);

    auto *layout = new QHBoxLayout(chip);
    layout->setContentsMargins(10, 2, 4, 2);
    layout->setSpacing(2);
    layout->addWidget(text);
    layout->addWidget(remove);
    return chip;
}

} // namespace

TagInput::TagInput(QWidget *parent)
    : QFrame(parent)
    , m_flow(new FlowLayout(this, 6))
    , m_edit(new QLineEdit(this))
    , m_model(new QStringListModel(this))
    , m_completer(new QCompleter(m_model, this))
{
    setObjectName("tagInput");
    m_flow->setContentsMargins(8, 6, 8, 6);
    // L'altezza dipende da quante righe di chip servono.
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    m_edit->setPlaceholderText(tr("Scrivi e premi Invio, es. vacanza"));
    m_edit->setMinimumWidth(140);
    m_edit->setMaxLength(TransactionService::kMaxTagLength + 1);
    m_edit->installEventFilter(this);

    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_edit->setCompleter(m_completer);

    m_flow->addWidget(m_edit);

    connect(m_edit, &QLineEdit::returnPressed, this, &TagInput::commitText);
    // Scelta dal menu dei suggerimenti: il testo va nel campo dopo il segnale, quindi un giro dopo.
    connect(m_completer, qOverload<const QString &>(&QCompleter::activated), this,
            [this] { QTimer::singleShot(0, this, &TagInput::commitText); });
    // La virgola separa le etichette mentre si scrive.
    connect(m_edit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (!text.contains(','))
            return;
        QStringList parts = text.split(',');
        const QString rest = parts.takeLast();
        for (const QString &part : std::as_const(parts))
            addTag(part);
        m_edit->setText(rest.trimmed());
    });
}

void TagInput::setTags(const QStringList &tags)
{
    m_tags.clear();
    m_edit->clear();
    for (const QString &t : tags)
        addTag(t);
    rebuild();
}

QStringList TagInput::tags() const
{
    QStringList result = m_tags;
    const QString pending = m_edit->text().trimmed();
    if (!pending.isEmpty())
        result.append(pending);
    return result;
}

void TagInput::setSuggestions(const QStringList &suggestions)
{
    m_model->setStringList(suggestions);
}

bool TagInput::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::KeyPress) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Backspace && m_edit->text().isEmpty() && !m_tags.isEmpty()) {
            removeTag(m_tags.last());
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void TagInput::addTag(QString tag)
{
    tag = tag.trimmed();
    while (tag.startsWith('#'))
        tag.remove(0, 1);
    tag = tag.simplified();
    if (tag.isEmpty() || m_tags.contains(tag, Qt::CaseInsensitive))
        return;
    m_tags.append(tag);
    rebuild();
}

void TagInput::removeTag(const QString &tag)
{
    m_tags.removeAll(tag);
    rebuild();
    m_edit->setFocus();
}

void TagInput::commitText()
{
    const QString text = m_edit->text();
    m_edit->clear();
    addTag(text);
}

void TagInput::rebuild()
{
    // Via i chip vecchi (il campo di testo resta, sempre in fondo).
    while (m_flow->count() > 0) {
        QLayoutItem *item = m_flow->takeAt(0);
        if (item->widget() && item->widget() != m_edit)
            item->widget()->deleteLater();
        delete item;
    }
    for (const QString &tag : std::as_const(m_tags))
        m_flow->addWidget(makeChip(tag, this, [this, tag] { removeTag(tag); }));
    m_flow->addWidget(m_edit);
    updateGeometry();
}
