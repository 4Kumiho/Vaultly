#include "ui/MainWindow.h"

#include "ui/HomePage.h"
#include "ui/LoginPage.h"
#include "ui/RegisterPage.h"
#include "ui/SlideStack.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stack(new SlideStack(this))
    , m_login(new LoginPage)
    , m_register(new RegisterPage)
    , m_home(new HomePage)
{
    setWindowTitle("Bankeviour");
    setMinimumSize(960, 640);
    resize(1180, 780);

    m_stack->addWidget(m_login);
    m_stack->addWidget(m_register);
    m_stack->addWidget(m_home);
    setCentralWidget(m_stack);
    m_login->setFocus();

    using Direction = SlideStack::Direction;

    const auto enterHome = [this](const Session &session) {
        setWindowTitle(QString("Bankeviour - %1").arg(session.user.username));
        m_home->setSession(session);
        m_stack->slideTo(m_home, Direction::Forward);
        m_login->reset();
        m_register->reset();
    };

    connect(m_login, &LoginPage::registerRequested, this, [this] {
        m_register->reset();
        m_stack->slideTo(m_register, Direction::Forward);
    });
    connect(m_register, &RegisterPage::loginRequested, this,
            [this] { m_stack->slideTo(m_login, Direction::Backward); });
    connect(m_login, &LoginPage::loggedIn, this, enterHome);
    connect(m_register, &RegisterPage::registered, this, enterHome);
    connect(m_home, &HomePage::logoutRequested, this, [this] {
        setWindowTitle("Bankeviour");
        m_login->reset();
        m_stack->slideTo(m_login, Direction::Backward);
    });
}
