#pragma once

#include "core/User.h"

#include <QWidget>

class ErrorLabel;
class QFrame;
class QLineEdit;
class QPushButton;

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);

    // Svuota i campi e gli errori (es. dopo il logout).
    void reset();

    // Mostra il link "Cerca aggiornamenti" (nascosto nelle build senza aggiornamenti).
    // Non "setUpdatesEnabled": in QWidget quel nome blocca il ridisegno.
    void setUpdateCheckAvailable(bool available);

signals:
    void loggedIn(const Session &session);
    void registerRequested();
    void checkUpdatesRequested();

private:
    void submit();

    QFrame *m_card;
    QPushButton *m_checkUpdates;
    QLineEdit *m_username;
    QLineEdit *m_password;
    ErrorLabel *m_error;
};
