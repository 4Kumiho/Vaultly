#pragma once

#include <QFrame>
#include <QLabel>

class QButtonGroup;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

// Piccoli mattoni riusati dalle pagine. Vedi Theme.h per ruoli e varianti.
namespace Components {

QLabel *label(const QString &text, const char *role, QWidget *parent = nullptr);
QPushButton *button(const QString &text, const char *variant, QWidget *parent = nullptr);

// Icona dell'app + "VAULTLY", in cima alle schede di login e registrazione.
QWidget *brand(QWidget *parent = nullptr);

// Riquadro con angoli arrotondati e ombra.
QFrame *card(QWidget *parent = nullptr);

// Etichetta sopra + campo sotto, aggiunti in coda al layout.
void addField(QVBoxLayout *layout, const QString &label, QWidget *field);

// Mette `content` al centro di `page`.
void centerIn(QWidget *page, QWidget *content);

// Da chiamare dopo aver cambiato una proprietà dinamica usata dal QSS.
void repolish(QWidget *widget);

} // namespace Components

// Etichetta d'errore che compare in dissolvenza.
class ErrorLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ErrorLabel(QWidget *parent = nullptr);

    void showMessage(const QString &message);
    void clearMessage();
};

// Pulsanti affiancati di cui uno solo è attivo (es. Entrata/Uscita, 1S/1M/1A).
class SegmentedControl : public QFrame
{
    Q_OBJECT

public:
    explicit SegmentedControl(QWidget *parent = nullptr);

    QPushButton *addSegment(const QString &text, const QString &toolTip = {});
    int currentIndex() const;
    void setCurrentIndex(int index);

signals:
    // Emesso solo quando è l'utente a cambiare segmento.
    void currentChanged(int index);

private:
    QButtonGroup *m_group;
    QHBoxLayout *m_layout;
};

// Riquadro cliccabile (usato per le schede dei conti).
class ClickableFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ClickableFrame(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_pressed = false;
};
