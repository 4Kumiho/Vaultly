#pragma once

#include "core/User.h"

#include <QWidget>

class DashboardPage;
class QLabel;
class SegmentedControl;
class SlideStack;
class Toast;
class VaultPage;

// Pagina dopo il login: intestazione (saluto, sezioni, logout) e sezioni "Conti" e "Password".
class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);

    void setSession(const Session &session);

signals:
    void logoutRequested();

private:
    void showSection(int index);
    void logout();

    QLabel *m_greeting;
    SegmentedControl *m_nav;
    SlideStack *m_sections;
    DashboardPage *m_dashboard;
    VaultPage *m_vault;
    Toast *m_toast;
};
