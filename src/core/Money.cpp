#include "core/Money.h"

namespace {

constexpr int kMaxDigits = 15; // resta ben dentro qint64

quint64 pow10(int n)
{
    quint64 r = 1;
    while (n-- > 0)
        r *= 10;
    return r;
}

} // namespace

QString Money::formatNumber(qint64 amount, int minorUnits, const QLocale &locale, bool grouping)
{
    const quint64 abs = amount < 0 ? quint64(0) - quint64(amount) : quint64(amount);
    const quint64 scale = pow10(minorUnits);

    // Raggruppamento a mano: QLocale in italiano non separa le migliaia sotto le 5 cifre
    // ("1234" ma "12.345"), e in una lista di importi l'effetto è incoerente.
    QString text = QString::number(abs / scale);
    if (grouping) {
        for (qsizetype i = text.size() - 3; i > 0; i -= 3)
            text.insert(i, locale.groupSeparator());
    }
    if (minorUnits > 0)
        text += locale.decimalPoint() + QString::number(abs % scale).rightJustified(minorUnits, '0');
    if (amount < 0)
        text.prepend(locale.negativeSign());
    return text;
}

QString Money::format(qint64 amount, const Currency &currency, const QLocale &locale)
{
    return formatNumber(amount, currency.minorUnits, locale) + ' ' + currency.symbol;
}

std::optional<qint64> Money::parse(const QString &text, int minorUnits)
{
    QString s = text.trimmed();
    s.remove(' ');
    s.remove(QChar(0x00A0)); // spazio non separabile, usato come separatore in alcune lingue

    bool negative = false;
    if (s.startsWith('-') || s.startsWith('+')) {
        negative = s.startsWith('-');
        s.remove(0, 1);
    }
    if (s.isEmpty())
        return std::nullopt;

    for (const QChar c : s) {
        if (!c.isDigit() && c != '.' && c != ',')
            return std::nullopt;
    }

    QString integerPart = s;
    QString fractionPart;
    const qsizetype lastSep = std::max(s.lastIndexOf('.'), s.lastIndexOf(','));
    if (lastSep >= 0 && minorUnits > 0) {
        const qsizetype digitsAfter = s.size() - lastSep - 1;
        if (digitsAfter >= 1 && digitsAfter <= minorUnits) {
            integerPart = s.left(lastSep);
            fractionPart = s.mid(lastSep + 1);
        }
    }
    integerPart.remove('.');
    integerPart.remove(',');
    if (fractionPart.contains('.') || fractionPart.contains(','))
        return std::nullopt;
    if (integerPart.isEmpty())
        integerPart = "0";
    if (integerPart.size() + minorUnits > kMaxDigits)
        return std::nullopt;

    const quint64 value = integerPart.toULongLong() * pow10(minorUnits)
        + fractionPart.leftJustified(minorUnits, '0').toULongLong();
    return negative ? -qint64(value) : qint64(value);
}
