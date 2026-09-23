#pragma once

#include "core/Currency.h"

#include <QLocale>

#include <optional>

// Importi sempre come interi nelle unità minime della valuta (es. centesimi).
namespace Money {

// 123456 con 2 decimali -> "1.234,56" (locale italiano). Senza separatore delle migliaia se grouping=false.
// Usa separatori e segno della locale, ma raggruppa sempre a gruppi di 3 cifre.
QString formatNumber(qint64 amount, int minorUnits, const QLocale &locale = QLocale(), bool grouping = true);

// 123456 in EUR -> "1.234,56 €"
QString format(qint64 amount, const Currency &currency, const QLocale &locale = QLocale());

// Interpreta un importo scritto dall'utente. Accetta sia '.' sia ',' come separatore decimale:
// l'ultimo separatore è decimale se seguito da al massimo `minorUnits` cifre, altrimenti è delle migliaia.
// "12,5" -> 1250, "1.234" -> 123400, "1.234,56" -> 123456, "-50" -> -5000.
std::optional<qint64> parse(const QString &text, int minorUnits);

} // namespace Money
