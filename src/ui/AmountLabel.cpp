#include "ui/AmountLabel.h"

#include "core/Money.h"
#include "ui/Components.h"

#include <QVariantAnimation>

AmountLabel::AmountLabel(QWidget *parent)
    : QLabel(parent)
    , m_anim(new QVariantAnimation(this))
{
    m_anim->setDuration(900);
    m_anim->setStartValue(0.0);
    m_anim->setEndValue(1.0);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &t) {
        render(m_from + qRound64(double(m_to - m_from) * t.toDouble()));
    });
}

void AmountLabel::setAmount(qint64 amount, const Currency &currency)
{
    // Stessa valuta: si parte dal valore mostrato. Valuta diversa: si riparte da zero.
    m_from = (m_hasValue && currency.code == m_currency.code) ? m_shown : 0;
    m_to = amount;
    m_currency = currency;
    m_hasValue = true;

    m_anim->stop();
    if (m_from == m_to)
        render(m_to);
    else
        m_anim->start();
}

void AmountLabel::render(qint64 value)
{
    m_shown = value;
    setText(Money::format(value, m_currency));

    const bool negative = value < 0;
    if (negative != m_negative) {
        m_negative = negative;
        setProperty("tone", negative ? "negative" : "");
        Components::repolish(this);
    }
}
