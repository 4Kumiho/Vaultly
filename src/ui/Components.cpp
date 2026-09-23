#include "ui/Components.h"

#include "ui/Animations.h"

#include <QButtonGroup>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

QLabel *Components::label(const QString &text, const char *role, QWidget *parent)
{
    auto *l = new QLabel(text, parent);
    l->setProperty("role", role);
    return l;
}

QPushButton *Components::button(const QString &text, const char *variant, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setProperty("variant", variant);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QFrame *Components::card(QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName("card");

    auto *shadow = new QGraphicsDropShadowEffect(frame);
    shadow->setBlurRadius(48);
    shadow->setOffset(0, 16);
    shadow->setColor(QColor(0, 0, 0, 150));
    frame->setGraphicsEffect(shadow);
    return frame;
}

void Components::addField(QVBoxLayout *layout, const QString &text, QWidget *field)
{
    QWidget *owner = layout->parentWidget();
    layout->addWidget(label(text, "fieldLabel", owner));
    layout->addWidget(field);
    layout->addSpacing(8);
}

void Components::centerIn(QWidget *page, QWidget *content)
{
    auto *grid = new QGridLayout(page);
    grid->addWidget(content, 0, 0, Qt::AlignCenter);
}

void Components::repolish(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

ErrorLabel::ErrorLabel(QWidget *parent)
    : QLabel(parent)
{
    setProperty("role", "error");
    setWordWrap(true);
    hide();
}

void ErrorLabel::showMessage(const QString &message)
{
    setText(message);
    if (isHidden()) {
        show();
        Animations::fadeIn(this, 220);
    }
}

void ErrorLabel::clearMessage()
{
    clear();
    hide();
}

SegmentedControl::SegmentedControl(QWidget *parent)
    : QFrame(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new QHBoxLayout(this))
{
    setObjectName("segmented");
    m_layout->setContentsMargins(3, 3, 3, 3);
    m_layout->setSpacing(2);
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, &SegmentedControl::currentChanged);
}

QPushButton *SegmentedControl::addSegment(const QString &text, const QString &toolTip)
{
    auto *b = Components::button(text, "segment", this);
    b->setCheckable(true);
    b->setToolTip(toolTip);
    const int id = m_group->buttons().size();
    m_group->addButton(b, id);
    m_layout->addWidget(b);
    if (id == 0)
        b->setChecked(true);
    return b;
}

int SegmentedControl::currentIndex() const
{
    return m_group->checkedId();
}

void SegmentedControl::setCurrentIndex(int index)
{
    if (QAbstractButton *b = m_group->button(index))
        b->setChecked(true);
}

ClickableFrame::ClickableFrame(QWidget *parent)
    : QFrame(parent)
{
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
}

void ClickableFrame::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = true;
    QFrame::mousePressEvent(event);
}

void ClickableFrame::mouseReleaseEvent(QMouseEvent *event)
{
    const bool click = m_pressed && event->button() == Qt::LeftButton
        && rect().contains(event->position().toPoint());
    m_pressed = false;
    QFrame::mouseReleaseEvent(event);
    if (click)
        emit clicked();
}
