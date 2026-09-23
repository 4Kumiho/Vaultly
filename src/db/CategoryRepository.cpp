#include "db/CategoryRepository.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

Category readCategory(const QSqlQuery &q)
{
    return {q.value(0).toLongLong(), q.value(1).toString(), transactionTypeFromDb(q.value(2).toString())};
}

} // namespace

QList<Category> CategoryRepository::all()
{
    QList<Category> result;
    // Ordine di inserimento: le categorie più comuni prima, "Altro" in fondo.
    QSqlQuery q("SELECT id, name, type FROM categories ORDER BY type, name = 'Altro', id");
    while (q.next())
        result.append(readCategory(q));
    return result;
}

std::optional<Category> CategoryRepository::find(qint64 categoryId)
{
    QSqlQuery q;
    q.prepare("SELECT id, name, type FROM categories WHERE id = ?");
    q.addBindValue(categoryId);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return readCategory(q);
}
