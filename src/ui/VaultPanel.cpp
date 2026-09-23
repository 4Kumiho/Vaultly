#include "ui/VaultPanel.h"

#include "core/VaultService.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

VaultPanel::VaultPanel(QWidget *parent)
    : SidePanel(parent)
{
    QFrame *d = drawer();

    m_title = Components::label({}, "title", d);
    auto *subtitle = Components::label(tr("Viene salvata cifrata: solo tu, con la tua password di accesso, "
                                          "puoi leggerla."),
                                       "subtitle", d);
    subtitle->setWordWrap(true);

    m_name = new QLineEdit(d);
    m_name->setPlaceholderText(tr("es. Gmail, Netflix, Banca"));
    m_url = new QLineEdit(d);
    m_url->setPlaceholderText(tr("Facoltativo, es. https://mail.google.com"));
    m_username = new QLineEdit(d);
    m_username->setPlaceholderText(tr("Username o email"));

    m_password = new QLineEdit(d);
    m_toggle = Components::button({}, "chip", d);
    auto *generate = Components::button(tr("Genera"), "chip", d);
    generate->setToolTip(tr("Crea una password casuale di 20 caratteri"));
    auto *passwordRow = new QHBoxLayout;
    passwordRow->setSpacing(8);
    passwordRow->addWidget(m_password, 1);
    passwordRow->addWidget(m_toggle);
    passwordRow->addWidget(generate);

    m_notes = new QPlainTextEdit(d);
    m_notes->setPlaceholderText(tr("Facoltative, es. domande di sicurezza, PIN..."));
    m_notes->setFixedHeight(90);
    m_notes->setTabChangesFocus(true);

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
    Components::addField(layout, tr("Sito web"), m_url);
    Components::addField(layout, tr("Username o email"), m_username);
    layout->addWidget(Components::label(tr("Password"), "fieldLabel", d));
    layout->addLayout(passwordRow);
    layout->addSpacing(8);
    Components::addField(layout, tr("Note"), m_notes);
    layout->addWidget(m_error);
    layout->addStretch();
    layout->addWidget(m_delete, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addWidget(cancelButton, 1);
    buttons->addWidget(saveButton, 1);
    layout->addLayout(buttons);

    connect(m_toggle, &QPushButton::clicked, this,
            [this] { setPasswordVisible(m_password->echoMode() == QLineEdit::Password); });
    connect(generate, &QPushButton::clicked, this, [this] {
        m_password->setText(VaultService::generatePassword());
        setPasswordVisible(true);
    });
    connect(cancelButton, &QPushButton::clicked, this, [this] { dismiss(); });
    connect(saveButton, &QPushButton::clicked, this, &VaultPanel::save);
    connect(m_delete, &QPushButton::clicked, this, &VaultPanel::deleteClicked);
    for (QLineEdit *field : {m_name, m_url, m_username, m_password})
        connect(field, &QLineEdit::returnPressed, this, &VaultPanel::save);
}

void VaultPanel::openForCreate(const Session &session)
{
    m_editing = false;
    m_session = session;
    m_entry = {};
    prepare(tr("Nuova password"));
    m_delete->hide();
    open(m_name);
}

void VaultPanel::openForEdit(const Session &session, const VaultEntry &entry)
{
    m_editing = true;
    m_session = session;
    m_entry = entry;
    prepare(tr("Modifica password"));
    m_delete->setText(tr("Elimina voce"));
    m_delete->show();
    open(m_name);
}

void VaultPanel::prepare(const QString &title)
{
    m_title->setText(title);
    m_name->setText(m_entry.title);
    m_url->setText(m_entry.url);
    m_username->setText(m_entry.username);
    m_password->setText(m_entry.password);
    m_notes->setPlainText(m_entry.notes);
    setPasswordVisible(false);
    m_confirmDelete = false;
    m_error->clearMessage();
}

void VaultPanel::setPasswordVisible(bool visible)
{
    m_password->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
    m_toggle->setText(visible ? tr("Nascondi") : tr("Mostra"));
}

void VaultPanel::save()
{
    VaultEntry e = m_entry;
    e.title = m_name->text();
    e.url = m_url->text();
    e.username = m_username->text();
    e.password = m_password->text();
    e.notes = m_notes->toPlainText();

    const auto result = m_editing ? VaultService::update(m_session, e) : VaultService::create(m_session, e);
    if (!result.entry) {
        m_error->showMessage(result.error);
        Animations::shake(drawer());
        return;
    }
    emit changed(m_editing ? tr("Voce aggiornata") : tr("Voce salvata"));
    dismiss();
}

void VaultPanel::deleteClicked()
{
    // Doppio click per confermare, senza finestre di dialogo.
    if (!m_confirmDelete) {
        m_confirmDelete = true;
        m_delete->setText(tr("Clicca di nuovo per confermare"));
        return;
    }
    if (!VaultService::remove(m_session, m_entry.id)) {
        m_error->showMessage(tr("Impossibile eliminare la voce."));
        return;
    }
    emit changed(tr("Voce eliminata"));
    dismiss();
}
