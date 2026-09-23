#include "ui/LoginPage.h"

#include "Version.h"
#include "core/AuthService.h"
#include "ui/Animations.h"
#include "ui/Components.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
    , m_card(Components::card(this))
{
    m_card->setFixedWidth(400);

    m_username = new QLineEdit(m_card);
    m_username->setPlaceholderText(tr("Il tuo username"));
    m_password = new QLineEdit(m_card);
    m_password->setPlaceholderText(tr("La tua password"));
    m_password->setEchoMode(QLineEdit::Password);
    m_error = new ErrorLabel(m_card);

    auto *submitButton = Components::button(tr("Accedi"), "primary", m_card);
    auto *registerButton = Components::button(tr("Registrati"), "link", m_card);

    auto *layout = new QVBoxLayout(m_card);
    layout->setContentsMargins(36, 36, 36, 30);
    layout->setSpacing(6);
    layout->addWidget(Components::brand(m_card));
    layout->addSpacing(6);
    layout->addWidget(Components::label(tr("Bentornato"), "title", m_card));
    layout->addWidget(Components::label(tr("Accedi per vedere i tuoi conti."), "subtitle", m_card));
    layout->addSpacing(22);
    Components::addField(layout, tr("Username"), m_username);
    Components::addField(layout, tr("Password"), m_password);
    layout->addWidget(m_error);
    layout->addSpacing(10);
    layout->addWidget(submitButton);
    layout->addSpacing(10);

    auto *footer = new QHBoxLayout;
    footer->addStretch();
    footer->addWidget(Components::label(tr("Non hai un account?"), "muted", m_card));
    footer->addWidget(registerButton);
    footer->addStretch();
    layout->addLayout(footer);

    // Versione e controllo manuale degli aggiornamenti.
    m_checkUpdates = Components::button(tr("Cerca aggiornamenti"), "link", m_card);
    m_checkUpdates->hide();
    auto *about = new QHBoxLayout;
    about->addStretch();
    about->addWidget(Components::label(tr("Versione %1").arg(VAULTLY_VERSION), "hint", m_card));
    about->addWidget(m_checkUpdates);
    about->addStretch();
    layout->addSpacing(6);
    layout->addLayout(about);
    connect(m_checkUpdates, &QPushButton::clicked, this, &LoginPage::checkUpdatesRequested);

    Components::centerIn(this, m_card);
    setFocusProxy(m_username);

    connect(submitButton, &QPushButton::clicked, this, &LoginPage::submit);
    connect(m_username, &QLineEdit::returnPressed, this, &LoginPage::submit);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginPage::submit);
    connect(registerButton, &QPushButton::clicked, this, &LoginPage::registerRequested);
}

void LoginPage::setUpdateCheckAvailable(bool available)
{
    m_checkUpdates->setVisible(available);
}

void LoginPage::reset()
{
    m_username->clear();
    m_password->clear();
    m_error->clearMessage();
}

void LoginPage::submit()
{
    const auto result = AuthService::login(m_username->text(), m_password->text());
    if (!result.user) {
        m_error->showMessage(result.error);
        Animations::shake(m_card);
        m_password->clear();
        m_password->setFocus();
        return;
    }
    emit loggedIn(result.session());
}
