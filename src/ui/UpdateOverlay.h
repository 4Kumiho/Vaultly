#pragma once

#include "core/Appcast.h"

#include <QWidget>

class QFrame;
class QLabel;
class QProgressBar;
class QPropertyAnimation;
class QPushButton;
class QTextBrowser;
class UpdateService;

// Schermata degli aggiornamenti: riquadro centrato sopra tutta la finestra, sfondo scurito.
// Segue gli stati di UpdateService: controllo, disponibile, download, installazione, errore.
class UpdateOverlay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    UpdateOverlay(UpdateService *service, QWidget *host);

    // Controllo richiesto dall'utente: la schermata si apre subito su "Controllo aggiornamenti…".
    void checkNow();

    qreal progress() const { return m_progress; }
    void setProgress(qreal progress);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum class State { Checking, UpToDate, Available, Downloading, Installing, Failed };

    void setState(State state);
    void showAnimated();
    void hideAnimated();
    void layoutCard();

    UpdateService *m_service;
    UpdateInfo m_update;
    State m_state = State::Checking;
    bool m_manual = false;
    bool m_failedWhileDownloading = false;
    qreal m_progress = 0.0;
    QPropertyAnimation *m_anim;

    QFrame *m_card;
    QLabel *m_icon;
    QLabel *m_title;
    QLabel *m_subtitle;
    QTextBrowser *m_notes;
    QProgressBar *m_bar;
    QLabel *m_barLabel;
    QPushButton *m_primary;
    QPushButton *m_secondary;
    QPushButton *m_skip;
};
