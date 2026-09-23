#pragma once

#include "core/Transaction.h"

#include <QList>

// Calcoli sull'andamento del saldo nel tempo. Il saldo iniziale del conto vale
// "prima di tutti i movimenti"; ogni movimento lo sposta nel suo istante.
namespace BalanceHistory {

struct Point
{
    QDateTime time;
    qint64 balance = 0;
};

struct Totals
{
    qint64 income = 0;  // somma delle entrate, positiva
    qint64 expense = 0; // somma delle uscite, positiva
};

enum class Period { Day, Week, Month, HalfYear, Year, All };

// Saldo includendo i movimenti avvenuti fino a `time` compreso.
qint64 balanceAt(qint64 initialBalance, const QList<Transaction> &transactions, const QDateTime &time);

// Serie "a gradini" tra `from` e `to`, ordinata nel tempo: un punto a `from`, due punti per
// ogni movimento nell'intervallo (saldo prima e dopo, stesso istante), un punto finale a `to`.
QList<Point> steps(qint64 initialBalance, const QList<Transaction> &transactions, const QDateTime &from,
                   const QDateTime &to);

// Entrate e uscite con data in [from, to].
Totals totals(const QList<Transaction> &transactions, const QDateTime &from, const QDateTime &to);

// Inizio del periodo che finisce a `now`. Per `All`: il movimento più vecchio
// (almeno un giorno prima di `now`), o un mese fa se non ci sono movimenti.
QDateTime periodStart(Period period, const QDateTime &now, const QList<Transaction> &transactions);

} // namespace BalanceHistory
