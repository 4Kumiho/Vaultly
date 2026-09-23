#pragma once

#include "core/Transaction.h"

#include <QList>

#include <optional>

namespace CategoryRepository {

// Tutte le categorie nell'ordine della lista predefinita, "Altro" in fondo a ciascun tipo.
QList<Category> all();

std::optional<Category> find(qint64 categoryId);

} // namespace CategoryRepository
