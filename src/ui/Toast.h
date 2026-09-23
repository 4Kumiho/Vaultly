#pragma once

#include <QLabel>
#include <QTimer>

class QGraphicsOpacityEffect;
class QPropertyAnimation;

// Messaggio breve in basso al centro della pagina ospite, che sparisce da solo.
class Toast : public QLabel
{
    Q_OBJECT

public:
    explicit Toast(QWidget *host);

    void showMessage(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void reposition();
    void fadeTo(qreal opacity);

    QGraphicsOpacityEffect *m_effect;
    QPropertyAnimation *m_anim;
    QTimer m_hideTimer;
};
