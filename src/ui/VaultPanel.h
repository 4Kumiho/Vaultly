#pragma once

#include "core/User.h"
#include "core/VaultEntry.h"
#include "ui/SidePanel.h"

class ErrorLabel;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

// Pannello laterale per aggiungere, modificare o eliminare una voce dell'area password.
class VaultPanel : public SidePanel
{
    Q_OBJECT

public:
    explicit VaultPanel(QWidget *parent);

    void openForCreate(const Session &session);
    void openForEdit(const Session &session, const VaultEntry &entry);

signals:
    // Una voce è stata salvata o eliminata; `message` è da mostrare all'utente.
    void changed(const QString &message);

private:
    void prepare(const QString &title);
    void setPasswordVisible(bool visible);
    void save();
    void deleteClicked();

    bool m_editing = false;
    bool m_confirmDelete = false;
    Session m_session;
    VaultEntry m_entry;

    QLabel *m_title;
    QLineEdit *m_name;
    QLineEdit *m_url;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QPushButton *m_toggle;
    QPlainTextEdit *m_notes;
    ErrorLabel *m_error;
    QPushButton *m_delete;
};
