#pragma once

#include "core/Transaction.h"

#include <QList>

// Quanto si spende per ogni etichetta in un periodo.
namespace TagStats {

struct Spending
{
    QString tag;      // vuoto per la voce "senza etichetta"
    qint64 total = 0; // somma delle uscite, positiva
    int count = 0;    // numero di uscite
};

struct Report
{
    QList<Spending> tags; // dalla spesa più alta alla più bassa
    Spending untagged;    // uscite senza etichette
};

// Solo le uscite con data in [from, to]. Un'uscita con più etichette conta per intero in ciascuna.
Report expensesByTag(const QList<Transaction> &transactions, const QDateTime &from, const QDateTime &to);

} // namespace TagStats
