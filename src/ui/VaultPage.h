#pragma once

#include "core/User.h"
#include "core/VaultEntry.h"

#include <QList>
#include <QWidget>

class QFrame;
class QLabel;
class QLineEdit;
class QTimer;
class QVBoxLayout;
class VaultPanel;

// Sezione "Password": elenco delle voci salvate, ricerca e copia negli appunti.
class VaultPage : public QWidget
{
    Q_OBJECT

public:
    // I pannelli laterali coprono `overlayHost` (tutta la finestra, intestazione compresa).
    explicit VaultPage(QWidget *overlayHost, QWidget *parent = nullptr);

    void setSession(const Session &session);
    // Logout: dimentica voci e chiave, svuota gli appunti se contengono qualcosa copiato da qui.
    void clearSession();
    void dismissPanels();

signals:
    void notify(const QString &message);

private:
    void reload(bool animate = true);
    void rebuildList(bool animate);
    void copyToClipboard(const QString &text, const QString &message);
    void clearClipboardIfOurs();

    Session m_session;
    QList<VaultEntry> m_entries;
    QString m_copiedText;

    QLineEdit *m_search;
    QLabel *m_count;
    QFrame *m_listCard;
    QVBoxLayout *m_listLayout;
    QFrame *m_emptyState;
    VaultPanel *m_panel;
    QTimer *m_clipboardTimer;
};
