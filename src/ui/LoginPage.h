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

signals:
    void loggedIn(const Session &session);
    void registerRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void submit();

    QFrame *m_card;
    QPushButton *m_checkUpdates;
    QLineEdit *m_username;
    QLineEdit *m_password;
    ErrorLabel *m_error;
};
