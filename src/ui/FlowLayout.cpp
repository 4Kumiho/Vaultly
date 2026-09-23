#include "ui/FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int spacing)
    : QLayout(parent)
    , m_spacing(spacing)
{
    setContentsMargins(0, 0, 0, 0);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem *item = takeAt(0))
        delete item;
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return int(m_items.size());
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem *item : m_items)
        size = size.expandedTo(item->minimumSize());
    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const QRect area = rect.marginsRemoved(contentsMargins());
    int x = area.x();
    int y = area.y();
    int lineHeight = 0;

    for (QLayoutItem *item : m_items) {
        if (item->widget() && item->widget()->isHidden())
            continue;
        const QSize hint = item->sizeHint();
        // L'ultimo elemento (il campo di testo) si allarga fino a fine riga.
        const bool last = item == m_items.last();
        if (x + hint.width() > area.right() + 1 && lineHeight > 0) {
            x = area.x();
            y += lineHeight + m_spacing;
            lineHeight = 0;
        }
        const int width = last ? std::max(hint.width(), area.right() + 1 - x) : hint.width();
        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), QSize(width, hint.height())));
        x += width + m_spacing;
        lineHeight = std::max(lineHeight, hint.height());
    }
    return y + lineHeight - rect.y() + contentsMargins().bottom();
}
