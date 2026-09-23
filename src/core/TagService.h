#pragma once

#include "core/Tag.h"

#include <QList>

#include <optional>

// Etichette create e gestite dall'utente: nome, scadenza, tetto di spesa.
namespace TagService {

struct Result
{
    std::optional<Tag> tag; // valorizzato solo in caso di successo
    QString error;          // messaggio da mostrare all'utente
};

QList<Tag> list(qint64 userId);

// Controlla nome (unico, max kMaxTagNameLength), date (per le scadenze con date: fine >= inizio)
// e tetto (maggiore di zero, con una valuta valida). Le impostazioni che non servono vengono azzerate.
Result create(qint64 userId, const Tag &tag);
Result update(qint64 userId, const Tag &tag);

// Elimina l'etichetta e la toglie da tutti i movimenti.
bool remove(qint64 userId, qint64 tagId);

// Spesa nel periodo in corso rispetto al tetto (Level::NoBudget se l'etichetta non ha tetto).
BudgetStatus status(qint64 userId, const Tag &tag, const QDateTime &now = QDateTime::currentDateTime());

} // namespace TagService
