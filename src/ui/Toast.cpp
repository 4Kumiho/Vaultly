#include "ui/Toast.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace {
constexpr int kVisibleMs = 2600;
constexpr int kBottomMargin = 32;
} // namespace

Toast::Toast(QWidget *host)
    : QLabel(host)
    , m_effect(new QGraphicsOpacityEffect(this))
    , m_anim(new QPropertyAnimation(m_effect, "opacity", this))
{
    setObjectName("toast");
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_effect->setOpacity(0.0);
    setGraphicsEffect(m_effect);
    hide();

    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, [this] {
        if (m_effect->opacity() <= 0.0)
            hide();
    });

    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, [this] { fadeTo(0.0); });

    host->installEventFilter(this);
}

void Toast::showMessage(const QString &message)
{
    setText(message);
    adjustSize();
    reposition();
    show();
    raise();
    fadeTo(1.0);
    m_hideTimer.start(kVisibleMs);
}

bool Toast::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize)
        reposition();
    return QLabel::eventFilter(watched, event);
}

void Toast::reposition()
{
    const QWidget *host = parentWidget();
    move((host->width() - width()) / 2, host->height() - height() - kBottomMargin);
}

void Toast::fadeTo(qreal opacity)
{
    m_anim->stop();
    m_anim->setStartValue(m_effect->opacity());
    m_anim->setEndValue(opacity);
    m_anim->start();
}
