#pragma once

#include "core/Currency.h"

#include <QLabel>

class QVariantAnimation;

// Etichetta con un importo che, quando cambia, "conta" fino al nuovo valore.
// Diventa rossa se l'importo è negativo.
class AmountLabel : public QLabel
{
    Q_OBJECT

public:
    explicit AmountLabel(QWidget *parent = nullptr);

    void setAmount(qint64 amount, const Currency &currency);

private:
    void render(qint64 value);

    QVariantAnimation *m_anim;
    Currency m_currency;
    qint64 m_from = 0;
    qint64 m_to = 0;
    qint64 m_shown = 0;
    bool m_hasValue = false;
    bool m_negative = false;
};
