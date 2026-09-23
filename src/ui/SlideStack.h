#pragma once

#include <QStackedWidget>

class QParallelAnimationGroup;

// QStackedWidget che cambia pagina facendola scorrere di lato.
class SlideStack : public QStackedWidget
{
    Q_OBJECT

public:
    enum class Direction { Forward, Backward };

    explicit SlideStack(QWidget *parent = nullptr);

    void slideTo(QWidget *page, Direction direction);

private:
    QParallelAnimationGroup *m_running = nullptr;
};
