#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

#include <optional>

// Scadenza di un'etichetta: su quale periodo si conta la spesa rispetto al tetto.
enum class TagPeriod {
    None,    // nessuna scadenza: da sempre
    Weekly,  // settimana in corso (lunedì–domenica), riparte ogni settimana
    Monthly, // mese in corso, riparte ogni mese
    Range,   // dal giorno `start` al giorno `end` compresi (es. un viaggio)
};

QString toDbString(TagPeriod period);
TagPeriod tagPeriodFromDb(const QString &value);

// Nome di etichetta ripulito: spazi in più e '#' iniziali tolti.
QString normalizeTagName(QString name);
constexpr int kMaxTagNameLength = 30;

// Etichetta dell'utente con le sue impostazioni.
struct Tag
{
    qint64 id = 0;
    QString name;
    TagPeriod period = TagPeriod::None;
    QDate start; // solo per Range
    QDate end;   // solo per Range
    std::optional<qint64> budget; // tetto di spesa in unità minime di `currencyCode`
    QString currencyCode;         // valuta del tetto (vuota se non c'è tetto)
    int usage = 0;                // movimenti che la usano (solo in lettura)
};

// Stato del tetto di un'etichetta nel suo periodo in corso.
struct BudgetStatus
{
    enum class Level { NoBudget, Ok, Warning, Over };

    QDateTime from; // inizio del periodo (invalido = da sempre)
    QDateTime to;   // fine del periodo
    qint64 spent = 0;
    qint64 budget = 0;
    Level level = Level::NoBudget;
    bool finished = false; // periodo con date già concluso
    bool upcoming = false; // periodo con date non ancora iniziato

    double ratio() const { return budget > 0 ? double(spent) / double(budget) : 0.0; }
};

namespace Budget {

// Da questa quota del tetto in su scatta l'avviso "quasi al limite".
constexpr double kWarningRatio = 0.8;

// Periodo in corso per la scadenza di `tag` rispetto a `now`: [from, to].
// None → from invalido (da sempre), to = fine dei tempi.
void window(const Tag &tag, const QDateTime &now, QDateTime *from, QDateTime *to);

BudgetStatus::Level levelFor(qint64 spent, qint64 budget);

} // namespace Budget
