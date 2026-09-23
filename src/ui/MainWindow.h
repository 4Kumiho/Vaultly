#pragma once

#include <QMainWindow>

class HomePage;
class LoginPage;
class RegisterPage;
class SlideStack;
class UpdateOverlay;
class UpdateService;

// Unica finestra dell'app: login, registrazione e home sono pagine che scorrono.
// Sopra a tutto, la schermata degli aggiornamenti.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Avvia i controlli automatici degli aggiornamenti (dopo aver mostrato la finestra).
    void startUpdateChecks();

private:
    SlideStack *m_stack;
    LoginPage *m_login;
    RegisterPage *m_register;
    HomePage *m_home;
    UpdateService *m_updates;
    UpdateOverlay *m_updateOverlay;
};
