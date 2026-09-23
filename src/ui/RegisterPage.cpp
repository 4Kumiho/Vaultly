#include "ui/RegisterPage.h"

#include "core/AuthService.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

RegisterPage::RegisterPage(QWidget *parent)
    : QWidget(parent)
    , m_card(Components::card(this))
{
    m_card->setFixedWidth(400);

    m_username = new QLineEdit(m_card);
    m_username->setPlaceholderText(tr("Scegli uno username"));
    m_password = new QLineEdit(m_card);
    m_password->setPlaceholderText(tr("Almeno %1 caratteri").arg(AuthService::kMinPasswordLength));
    m_password->setEchoMode(QLineEdit::Password);
    m_confirm = new QLineEdit(m_card);
    m_confirm->setPlaceholderText(tr("Ripeti la password"));
    m_confirm->setEchoMode(QLineEdit::Password);
    m_error = new ErrorLabel(m_card);

    auto *submitButton = Components::button(tr("Crea account"), "primary", m_card);
    auto *loginButton = Components::button(tr("Accedi"), "link", m_card);

    auto *layout = new QVBoxLayout(m_card);
    layout->setContentsMargins(36, 36, 36, 30);
    layout->setSpacing(6);
    layout->addWidget(Components::label("VAULTLY", "brand", m_card));
    layout->addSpacing(6);
    layout->addWidget(Components::label(tr("Crea il tuo account"), "title", m_card));
    layout->addWidget(Components::label(tr("Bastano uno username e una password."), "subtitle", m_card));
    layout->addSpacing(22);
    Components::addField(layout, tr("Username"), m_username);
    Components::addField(layout, tr("Password"), m_password);
    Components::addField(layout, tr("Conferma password"), m_confirm);
    layout->addWidget(m_error);
    layout->addSpacing(10);
    layout->addWidget(submitButton);
    layout->addSpacing(10);

    auto *footer = new QHBoxLayout;
    footer->addStretch();
    footer->addWidget(Components::label(tr("Hai già un account?"), "muted", m_card));
    footer->addWidget(loginButton);
    footer->addStretch();
    layout->addLayout(footer);

    Components::centerIn(this, m_card);
    setFocusProxy(m_username);

    connect(submitButton, &QPushButton::clicked, this, &RegisterPage::submit);
    for (QLineEdit *field : {m_username, m_password, m_confirm})
        connect(field, &QLineEdit::returnPressed, this, &RegisterPage::submit);
    connect(loginButton, &QPushButton::clicked, this, &RegisterPage::loginRequested);
}

void RegisterPage::reset()
{
    m_username->clear();
    m_password->clear();
    m_confirm->clear();
    m_error->clearMessage();
}

void RegisterPage::submit()
{
    const auto result = AuthService::registerUser(m_username->text(), m_password->text(), m_confirm->text());
    if (!result.user) {
        m_error->showMessage(result.error);
        Animations::shake(m_card);
        return;
    }
    emit registered(result.session());
}
