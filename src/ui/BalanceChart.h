#pragma once

#include "core/BalanceHistory.h"
#include "core/Currency.h"

#include <QChartView>

class QAreaSeries;
class QDateTimeAxis;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QLineSeries;
class QValueAxis;

// Grafico del saldo nel tempo.
// - rotella: zoom sull'asse del tempo, centrato sul mouse
// - trascinamento: spostamento nel tempo
// - doppio click: torna al periodo scelto
// - passando col mouse: saldo in quel momento
class BalanceChart : public QChartView
{
    Q_OBJECT

public:
    explicit BalanceChart(QWidget *parent = nullptr);

    // `points` copre tutto lo storico disponibile; [from, to] è l'intervallo mostrato all'inizio.
    void setData(const QList<BalanceHistory::Point> &points, const Currency &currency, const QDateTime &from,
                 const QDateTime &to);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setVisibleRange(qint64 minMs, qint64 maxMs);
    void fitValueAxis();
    qint64 balanceAt(qint64 ms) const;
    double toUnits(qint64 amount) const;
    void showHover(const QPointF &viewPos);
    void hideHover();

    QList<BalanceHistory::Point> m_points;
    Currency m_currency;
    qint64 m_dataMin = 0;
    qint64 m_dataMax = 0;
    qint64 m_defaultMin = 0;
    qint64 m_defaultMax = 0;

    QLineSeries *m_line; // bordo superiore dell'area (non disegnato)
    QAreaSeries *m_area;
    QLineSeries *m_stroke; // linea visibile, stessi punti di m_line
    QDateTimeAxis *m_timeAxis;
    QValueAxis *m_valueAxis;
    QGraphicsLineItem *m_hoverLine;
    QGraphicsEllipseItem *m_hoverDot;

    bool m_dragging = false;
    QPointF m_lastDragPos;
};
