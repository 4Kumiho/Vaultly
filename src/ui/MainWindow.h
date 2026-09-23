#pragma once

#include <QMainWindow>

class HomePage;
class LoginPage;
class RegisterPage;
class SlideStack;

// Unica finestra dell'app: login, registrazione e home sono pagine che scorrono.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    SlideStack *m_stack;
    LoginPage *m_login;
    RegisterPage *m_register;
    HomePage *m_home;
};
