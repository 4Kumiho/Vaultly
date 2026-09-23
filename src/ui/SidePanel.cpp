#include "ui/SidePanel.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QPainter>
#include <QPropertyAnimation>

namespace {
constexpr int kDrawerWidth = 420;
constexpr int kBackdropAlpha = 150;
} // namespace

SidePanel::SidePanel(QWidget *parent)
    : QWidget(parent)
    , m_anim(new QPropertyAnimation(this, "progress", this))
    , m_drawer(new QFrame(this))
{
    hide();
    parent->installEventFilter(this);

    m_anim->setDuration(340);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, [this] {
        if (m_progress <= 0.0)
            hide();
    });

    m_drawer->setObjectName("drawer");
    auto *shadow = new QGraphicsDropShadowEffect(m_drawer);
    shadow->setBlurRadius(60);
    shadow->setOffset(-10, 0);
    shadow->setColor(QColor(0, 0, 0, 170));
    m_drawer->setGraphicsEffect(shadow);
}

void SidePanel::open(QWidget *focusWidget)
{
    setGeometry(parentWidget()->rect());
    show();
    raise();
    animateTo(1.0);
    if (focusWidget)
        focusWidget->setFocus();
}

void SidePanel::dismiss(bool animated)
{
    if (isHidden())
        return;
    if (!animated) {
        m_anim->stop();
        setProgress(0.0);
        hide();
        return;
    }
    animateTo(0.0);
}

void SidePanel::animateTo(qreal target)
{
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(target);
    m_anim->start();
}

void SidePanel::setProgress(qreal progress)
{
    m_progress = progress;
    layoutDrawer();
    update();
}

void SidePanel::layoutDrawer()
{
    const int w = qMin(kDrawerWidth, width());
    const int x = width() - qRound(w * m_progress);
    m_drawer->setGeometry(x, 0, w, height());
}

bool SidePanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize)
        setGeometry(parentWidget()->rect());
    return QWidget::eventFilter(watched, event);
}

void SidePanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutDrawer();
}

void SidePanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, qRound(kBackdropAlpha * m_progress)));
}

void SidePanel::mousePressEvent(QMouseEvent *event)
{
    // Click sullo sfondo scuro, fuori dal pannello: si chiude.
    if (!m_drawer->geometry().contains(event->position().toPoint()))
        dismiss();
}

void SidePanel::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        dismiss();
    else
        QWidget::keyPressEvent(event);
}
