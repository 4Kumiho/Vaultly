#pragma once

#include <QWidget>

class QFrame;
class QPropertyAnimation;

// Base per i pannelli laterali: copre la pagina genitore con uno sfondo scuro
// semitrasparente e fa entrare da destra il riquadro `drawer()`, dove le
// sottoclassi mettono il loro modulo. Si chiude con Esc o cliccando fuori.
class SidePanel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit SidePanel(QWidget *parent);

    void dismiss(bool animated = true);

    qreal progress() const { return m_progress; }
    void setProgress(qreal progress);

protected:
    QFrame *drawer() const { return m_drawer; }

    // Mostra il pannello con l'animazione e mette il focus su `focusWidget`.
    void open(QWidget *focusWidget);

    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void animateTo(qreal target);
    void layoutDrawer();

    qreal m_progress = 0.0;
    QPropertyAnimation *m_anim;
    QFrame *m_drawer;
};
