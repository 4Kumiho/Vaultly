#pragma once

#include <QString>

struct Currency
{
    QString code;       // ISO 4217, es. "EUR"
    QString symbol;     // es. "€"
    int minorUnits = 2; // cifre decimali: 2 per EUR, 0 per JPY
};
