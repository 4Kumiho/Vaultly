#include "ui/BalanceChart.h"

#include "core/Money.h"

#include <QAreaSeries>
#include <QDateTimeAxis>
#include <QEasingCurve>
#include <QGraphicsEllipseItem>
#include <QGraphicsLayout>
#include <QGraphicsLineItem>
#include <QLineSeries>
#include <QToolTip>
#include <QValueAxis>

#include <algorithm>
#include <cmath>

namespace {

constexpr qint64 kMinSpanMs = 60LL * 60 * 1000; // zoom massimo: un'ora
constexpr qint64 kDayMs = 24LL * 60 * 60 * 1000;

const QColor kAccent("#5b8cff");
const QColor kAxisText("#8b93a7");
const QColor kGrid("#20242d");

QString timeFormatFor(qint64 spanMs)
{
    if (spanMs <= 2 * kDayMs)
        return "HH:mm";
    if (spanMs <= 90 * kDayMs)
        return "d MMM";
    return "MMM yyyy";
}

} // namespace

BalanceChart::BalanceChart(QWidget *parent)
    : QChartView(parent)
    , m_line(new QLineSeries)
    , m_area(new QAreaSeries(m_line))
    , m_stroke(new QLineSeries)
    , m_timeAxis(new QDateTimeAxis)
    , m_valueAxis(new QValueAxis)
{
    auto *chart = new QChart;
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->legend()->hide();
    chart->setMargins(QMargins(0, 8, 8, 0));
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setLocalizeNumbers(true);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setAnimationDuration(600);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);

    // Sfumatura sotto la linea del saldo. L'area non ha bordo (sarebbe disegnato anche
    // sui lati e sul fondo): la linea è una serie a parte con gli stessi punti.
    m_area->setPen(Qt::NoPen);
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    QColor top = kAccent;
    top.setAlpha(110);
    QColor bottom = kAccent;
    bottom.setAlpha(0);
    gradient.setColorAt(0.0, top);
    gradient.setColorAt(1.0, bottom);
    m_area->setBrush(gradient);
    chart->addSeries(m_area);
    m_stroke->setPen(QPen(kAccent, 2.5));
    chart->addSeries(m_stroke);

    QFont axisFont = font();
    axisFont.setPointSizeF(8.5);
    for (QAbstractAxis *axis : {static_cast<QAbstractAxis *>(m_timeAxis), static_cast<QAbstractAxis *>(m_valueAxis)}) {
        axis->setLabelsColor(kAxisText);
        axis->setLabelsFont(axisFont);
        axis->setGridLineColor(kGrid);
        axis->setLinePenColor(kGrid);
        axis->setMinorGridLineVisible(false);
    }
    m_timeAxis->setTickCount(6);
    m_timeAxis->setGridLineVisible(false);
    m_valueAxis->setTickCount(5);
    chart->addAxis(m_timeAxis, Qt::AlignBottom);
    chart->addAxis(m_valueAxis, Qt::AlignLeft);
    for (QAbstractSeries *series : {static_cast<QAbstractSeries *>(m_area), static_cast<QAbstractSeries *>(m_stroke)}) {
        series->attachAxis(m_timeAxis);
        series->attachAxis(m_valueAxis);
    }

    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(Qt::NoBrush);
    setStyleSheet("background: transparent;");
    viewport()->setAutoFillBackground(false);
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);

    // Linea verticale e pallino che seguono il mouse.
    m_hoverLine = new QGraphicsLineItem(chart);
    m_hoverLine->setPen(QPen(QColor(255, 255, 255, 60), 1, Qt::DashLine));
    m_hoverDot = new QGraphicsEllipseItem(-5, -5, 10, 10, chart);
    m_hoverDot->setBrush(kAccent);
    m_hoverDot->setPen(QPen(QColor("#0f1115"), 2));
    m_hoverLine->setZValue(10);
    m_hoverDot->setZValue(11);
    hideHover();
}

void BalanceChart::setData(const QList<BalanceHistory::Point> &points, const Currency &currency,
                           const QDateTime &from, const QDateTime &to)
{
    m_points = points;
    m_currency = currency;

    QList<QPointF> data;
    data.reserve(points.size());
    for (const auto &p : points)
        data.append(QPointF(p.time.toMSecsSinceEpoch(), toUnits(p.balance)));
    m_line->replace(data);
    m_stroke->replace(data);

    m_dataMin = points.isEmpty() ? from.toMSecsSinceEpoch() : points.first().time.toMSecsSinceEpoch();
    m_dataMax = points.isEmpty() ? to.toMSecsSinceEpoch() : points.last().time.toMSecsSinceEpoch();
    m_defaultMin = from.toMSecsSinceEpoch();
    m_defaultMax = to.toMSecsSinceEpoch();
    setVisibleRange(m_defaultMin, m_defaultMax);
    hideHover();
}

void BalanceChart::setVisibleRange(qint64 minMs, qint64 maxMs)
{
    // Lo span resta tra un'ora e tutto lo storico; la finestra non esce dai dati.
    const qint64 fullSpan = std::max(m_dataMax - m_dataMin, kMinSpanMs);
    const qint64 span = std::clamp(maxMs - minMs, kMinSpanMs, fullSpan);
    minMs = std::clamp(minMs, m_dataMin, std::max(m_dataMin, m_dataMax - span));
    maxMs = minMs + span;

    m_timeAxis->setFormat(timeFormatFor(span));
    m_timeAxis->setRange(QDateTime::fromMSecsSinceEpoch(minMs), QDateTime::fromMSecsSinceEpoch(maxMs));
    fitValueAxis();
}

void BalanceChart::fitValueAxis()
{
    const qint64 minMs = m_timeAxis->min().toMSecsSinceEpoch();
    const qint64 maxMs = m_timeAxis->max().toMSecsSinceEpoch();

    // Estremi del saldo nella finestra visibile (compresi i bordi).
    qint64 lo = balanceAt(minMs);
    qint64 hi = lo;
    for (const auto &p : std::as_const(m_points)) {
        const qint64 t = p.time.toMSecsSinceEpoch();
        if (t >= minMs && t <= maxMs) {
            lo = std::min(lo, p.balance);
            hi = std::max(hi, p.balance);
        }
    }
    const qint64 atEnd = balanceAt(maxMs);
    lo = std::min(lo, atEnd);
    hi = std::max(hi, atEnd);

    double low = toUnits(lo);
    double high = toUnits(hi);
    const double pad = high > low ? (high - low) * 0.15 : std::max(std::abs(high) * 0.1, 1.0);
    low -= pad;
    high += pad;
    m_valueAxis->setRange(low, high);
    m_valueAxis->applyNiceNumbers();
    m_valueAxis->setLabelFormat(m_valueAxis->max() - m_valueAxis->min() < 10 ? "%.2f" : "%.0f");
}

qint64 BalanceChart::balanceAt(qint64 ms) const
{
    // I punti sono a gradini e ordinati: vale l'ultimo punto non successivo a `ms`.
    if (m_points.isEmpty())
        return 0;
    qint64 balance = m_points.first().balance;
    for (const auto &p : m_points) {
        if (p.time.toMSecsSinceEpoch() > ms)
            break;
        balance = p.balance;
    }
    return balance;
}

double BalanceChart::toUnits(qint64 amount) const
{
    // Solo per disegnare: i calcoli restano sugli interi.
    return double(amount) / std::pow(10.0, m_currency.minorUnits);
}

void BalanceChart::wheelEvent(QWheelEvent *event)
{
    const QRectF plot = chart()->plotArea();
    if (event->angleDelta().y() == 0 || plot.width() <= 0) {
        event->ignore();
        return;
    }

    const qint64 minMs = m_timeAxis->min().toMSecsSinceEpoch();
    const qint64 maxMs = m_timeAxis->max().toMSecsSinceEpoch();
    const double ratio = std::clamp((event->position().x() - plot.left()) / plot.width(), 0.0, 1.0);
    const double anchor = minMs + (maxMs - minMs) * ratio;
    const double factor = event->angleDelta().y() > 0 ? 0.8 : 1.25;
    const double span = (maxMs - minMs) * factor;

    const qint64 newMin = qint64(anchor - span * ratio);
    setVisibleRange(newMin, newMin + qint64(span));
    showHover(event->position());
    event->accept();
}

void BalanceChart::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastDragPos = event->position();
        setCursor(Qt::ClosedHandCursor);
        hideHover();
    }
    event->accept();
}

void BalanceChart::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        showHover(event->position());
        return;
    }
    const QRectF plot = chart()->plotArea();
    const qint64 minMs = m_timeAxis->min().toMSecsSinceEpoch();
    const qint64 maxMs = m_timeAxis->max().toMSecsSinceEpoch();
    const double dx = event->position().x() - m_lastDragPos.x();
    m_lastDragPos = event->position();
    if (plot.width() <= 0)
        return;
    const qint64 shift = qint64(-dx * (maxMs - minMs) / plot.width());
    setVisibleRange(minMs + shift, maxMs + shift);
}

void BalanceChart::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        showHover(event->position());
    }
    event->accept();
}

void BalanceChart::mouseDoubleClickEvent(QMouseEvent *event)
{
    setVisibleRange(m_defaultMin, m_defaultMax);
    event->accept();
}

void BalanceChart::leaveEvent(QEvent *event)
{
    hideHover();
    QChartView::leaveEvent(event);
}

void BalanceChart::resizeEvent(QResizeEvent *event)
{
    QChartView::resizeEvent(event);
    hideHover();
}

void BalanceChart::showHover(const QPointF &viewPos)
{
    const QPointF chartPos = chart()->mapFromScene(mapToScene(viewPos.toPoint()));
    const QRectF plot = chart()->plotArea();
    if (m_points.isEmpty() || !plot.contains(chartPos)) {
        hideHover();
        return;
    }

    const qint64 ms = qint64(chart()->mapToValue(chartPos, m_area).x());
    const qint64 balance = balanceAt(ms);
    const QPointF dot = chart()->mapToPosition(QPointF(ms, toUnits(balance)), m_area);

    m_hoverLine->setLine(chartPos.x(), plot.top(), chartPos.x(), plot.bottom());
    m_hoverDot->setPos(chartPos.x(), std::clamp(dot.y(), plot.top(), plot.bottom()));
    m_hoverLine->show();
    m_hoverDot->show();

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(ms);
    const QString when = QLocale().toString(time, "d MMM yyyy, HH:mm");
    QToolTip::showText(mapToGlobal(viewPos.toPoint()) + QPoint(14, -40),
                       QString("<b>%1</b><br>%2").arg(Money::format(balance, m_currency), when), this);
}

void BalanceChart::hideHover()
{
    m_hoverLine->hide();
    m_hoverDot->hide();
    QToolTip::hideText();
}
