#include "core/TagService.h"

#include "db/CurrencyRepository.h"
#include "db/TagRepository.h"

#include <QCoreApplication>

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("TagService", text);
}

TagService::Result failure(const QString &message)
{
    return {std::nullopt, message};
}

bool currencyExists(const QString &code)
{
    for (const Currency &c : CurrencyRepository::all()) {
        if (c.code == code)
            return true;
    }
    return false;
}

// Ripulisce `t` e restituisce un messaggio d'errore, o una stringa vuota se va bene.
QString normalize(qint64 userId, Tag &t)
{
    t.name = normalizeTagName(t.name);
    if (t.name.isEmpty())
        return tr("Inserisci un nome per l'etichetta.");
    if (t.name.size() > kMaxTagNameLength)
        return tr("Il nome può avere al massimo %1 caratteri.").arg(kMaxTagNameLength);
    if (TagRepository::nameExists(userId, t.name, t.id))
        return tr("Hai già un'etichetta con questo nome.");

    if (t.period == TagPeriod::Range) {
        if (!t.start.isValid() || !t.end.isValid())
            return tr("Scegli le date di inizio e di fine.");
        if (t.end < t.start)
            return tr("La data di fine deve essere uguale o successiva a quella di inizio.");
    } else {
        t.start = {};
        t.end = {};
    }

    if (t.budget) {
        if (*t.budget <= 0)
            return tr("Il tetto di spesa deve essere maggiore di zero.");
        if (!currencyExists(t.currencyCode))
            return tr("Scegli la valuta del tetto di spesa.");
    } else {
        t.currencyCode.clear();
    }
    return {};
}

} // namespace

QList<Tag> TagService::list(qint64 userId)
{
    return TagRepository::listForUser(userId);
}

TagService::Result TagService::create(qint64 userId, const Tag &tag)
{
    Tag t = tag;
    t.id = 0;
    if (const QString error = normalize(userId, t); !error.isEmpty())
        return failure(error);
    const auto id = TagRepository::insert(userId, t);
    if (!id)
        return failure(tr("Impossibile salvare l'etichetta."));
    t.id = *id;
    return {t, {}};
}

TagService::Result TagService::update(qint64 userId, const Tag &tag)
{
    if (!TagRepository::find(tag.id, userId))
        return failure(tr("Etichetta non trovata."));
    Tag t = tag;
    if (const QString error = normalize(userId, t); !error.isEmpty())
        return failure(error);
    if (!TagRepository::update(t, userId))
        return failure(tr("Impossibile salvare l'etichetta."));
    return {t, {}};
}

bool TagService::remove(qint64 userId, qint64 tagId)
{
    return TagRepository::remove(tagId, userId);
}

BudgetStatus TagService::status(qint64 userId, const Tag &tag, const QDateTime &now)
{
    BudgetStatus s;
    Budget::window(tag, now, &s.from, &s.to);
    if (tag.period == TagPeriod::Range) {
        s.finished = now > s.to;
        s.upcoming = now < s.from;
    }
    if (!tag.budget)
        return s;
    s.budget = *tag.budget;
    s.spent = TagRepository::spent(userId, tag.id, tag.currencyCode, s.from, s.to);
    s.level = Budget::levelFor(s.spent, s.budget);
    return s;
}
