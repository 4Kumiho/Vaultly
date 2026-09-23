#pragma once

#include "core/Currency.h"

struct Account
{
    qint64 id = 0;
    qint64 userId = 0;
    QString name;
    Currency currency;
    qint64 initialBalance = 0; // in unità minime della valuta
};
