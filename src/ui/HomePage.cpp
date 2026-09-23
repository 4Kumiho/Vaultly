#include "ui/HomePage.h"

#include "ui/Components.h"
#include "ui/DashboardPage.h"
#include "ui/SlideStack.h"
#include "ui/Toast.h"
#include "ui/VaultPage.h"

#include <QDate>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
// Indici delle sezioni, uguali nel selettore e nello stack.
constexpr int kAccounts = 0;
constexpr int kVault = 1;
} // namespace

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , m_greeting(Components::label({}, "title", this))
    , m_nav(new SegmentedControl(this))
    , m_sections(new SlideStack(this))
{
    QString today = QLocale().toString(QDate::currentDate(), "dddd d MMMM yyyy");
    if (!today.isEmpty())
        today[0] = today[0].toUpper();
    auto *date = Components::label(today, "subtitle", this);

    m_nav->addSegment(tr("Conti"));
    m_nav->addSegment(tr("Password"));
    auto *logoutButton = Components::button(tr("Esci"), "secondary", this);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addWidget(m_greeting);
    titles->addWidget(date);

    auto *header = new QHBoxLayout;
    header->setContentsMargins(44, 32, 44, 12);
    header->setSpacing(16);
    header->addLayout(titles, 1);
    header->addWidget(m_nav, 0, Qt::AlignVCenter);
    header->addWidget(logoutButton, 0, Qt::AlignVCenter);

    // I pannelli laterali delle sezioni coprono tutta questa pagina.
    m_dashboard = new DashboardPage(this);
    m_vault = new VaultPage(this);
    m_sections->addWidget(m_dashboard);
    m_sections->addWidget(m_vault);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addLayout(header);
    layout->addWidget(m_sections, 1);

    m_toast = new Toast(this);

    connect(m_nav, &SegmentedControl::currentChanged, this, &HomePage::showSection);
    connect(logoutButton, &QPushButton::clicked, this, &HomePage::logout);
    connect(m_vault, &VaultPage::notify, m_toast, [this](const QString &message) { m_toast->showMessage(message); });
    connect(m_dashboard, &DashboardPage::notify, m_toast, &Toast::showMessage);
}

void HomePage::setSession(const Session &session)
{
    m_greeting->setText(tr("Ciao, %1").arg(session.user.username));
    m_nav->setCurrentIndex(kAccounts);
    m_sections->setCurrentIndex(kAccounts);
    m_dashboard->setUser(session.user);
    m_vault->setSession(session);
}

void HomePage::showSection(int index)
{
    QWidget *target = index == kVault ? static_cast<QWidget *>(m_vault) : m_dashboard;
    const int current = m_sections->currentIndex();
    m_sections->slideTo(target, index > current ? SlideStack::Direction::Forward : SlideStack::Direction::Backward);
}

void HomePage::logout()
{
    m_dashboard->dismissPanels();
    m_vault->dismissPanels();
    m_vault->clearSession();
    emit logoutRequested();
}
