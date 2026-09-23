#include "ui/SlideStack.h"

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

SlideStack::SlideStack(QWidget *parent)
    : QStackedWidget(parent)
{
}

void SlideStack::slideTo(QWidget *page, Direction direction)
{
    // Se una transizione è ancora in corso la si porta subito alla fine.
    if (m_running)
        m_running->setCurrentTime(m_running->totalDuration());

    QWidget *from = currentWidget();
    if (!page || page == from)
        return;

    const int offset = direction == Direction::Forward ? width() : -width();
    page->setGeometry(0, 0, width(), height());
    page->move(offset, 0);
    page->show();
    page->raise();

    auto *group = new QParallelAnimationGroup(this);
    const auto addSlide = [group](QWidget *w, QPoint start, QPoint end) {
        auto *anim = new QPropertyAnimation(w, "pos");
        anim->setDuration(420);
        anim->setStartValue(start);
        anim->setEndValue(end);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        group->addAnimation(anim);
    };
    addSlide(from, QPoint(0, 0), QPoint(-offset, 0));
    addSlide(page, QPoint(offset, 0), QPoint(0, 0));

    m_running = group;
    connect(group, &QAbstractAnimation::finished, this, [this, group, from, page] {
        setCurrentWidget(page);
        from->move(0, 0);
        if (m_running == group)
            m_running = nullptr;
        group->deleteLater();
        page->setFocus();
    });
    group->start();
}
