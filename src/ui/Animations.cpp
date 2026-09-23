#include "ui/Animations.h"

#include <QGraphicsOpacityEffect>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

void Animations::fadeIn(QWidget *widget, int durationMs, int delayMs)
{
    auto *effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);

    auto *anim = new QPropertyAnimation(effect, "opacity", effect);
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    // A fine animazione si toglie l'effetto (costa in rendering). Lo si fa fuori dal
    // segnale finished, perché rimuovere l'effetto distrugge anche l'animazione.
    QPointer<QGraphicsOpacityEffect> guard(effect);
    QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget, guard] {
        QTimer::singleShot(0, widget, [widget, guard] {
            if (guard && widget->graphicsEffect() == guard)
                widget->setGraphicsEffect(nullptr);
        });
    });

    if (delayMs > 0)
        QTimer::singleShot(delayMs, anim, [anim] { anim->start(); });
    else
        anim->start();
}

void Animations::shake(QWidget *widget)
{
    if (widget->property("_shaking").toBool())
        return;
    widget->setProperty("_shaking", true);

    const QPoint origin = widget->pos();
    auto *anim = new QPropertyAnimation(widget, "pos", widget);
    anim->setDuration(420);
    anim->setKeyValueAt(0.00, origin);
    anim->setKeyValueAt(0.15, origin + QPoint(-10, 0));
    anim->setKeyValueAt(0.30, origin + QPoint(10, 0));
    anim->setKeyValueAt(0.45, origin + QPoint(-7, 0));
    anim->setKeyValueAt(0.60, origin + QPoint(5, 0));
    anim->setKeyValueAt(0.75, origin + QPoint(-2, 0));
    anim->setKeyValueAt(1.00, origin);
    QObject::connect(anim, &QPropertyAnimation::finished, widget,
                     [widget] { widget->setProperty("_shaking", false); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
