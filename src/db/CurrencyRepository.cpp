#include "db/CurrencyRepository.h"

#include <QSqlQuery>
#include <QVariant>

QList<Currency> CurrencyRepository::all()
{
    QList<Currency> result;
    QSqlQuery q("SELECT code, symbol, minor_units FROM currencies ORDER BY code");
    while (q.next())
        result.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toInt()});
    return result;
}
