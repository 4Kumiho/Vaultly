#pragma once

#include "core/User.h"

#include <QWidget>

class ErrorLabel;
class QFrame;
class QLineEdit;

class RegisterPage : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterPage(QWidget *parent = nullptr);

    // Svuota i campi e gli errori.
    void reset();

signals:
    void registered(const Session &session);
    void loginRequested();

private:
    void submit();

    QFrame *m_card;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLineEdit *m_confirm;
    ErrorLabel *m_error;
};
