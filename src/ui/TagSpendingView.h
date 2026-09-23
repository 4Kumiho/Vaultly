#pragma once

#include "core/Currency.h"
#include "core/Tag.h"
#include "core/TagStats.h"

#include <QList>
#include <QWidget>

#include <optional>

class QVBoxLayout;

// Riquadro delle etichette. Per ognuna:
// - con tetto: speso nel periodo della sua scadenza / tetto, barra verde-gialla-rossa e stato;
// - senza tetto: quanto si è speso nel periodo e conto scelti nella dashboard.
// In fondo le uscite senza etichetta. Clic su una riga = filtro della lista movimenti.
class TagSpendingView : public QWidget
{
    Q_OBJECT

public:
    struct Item
    {
        Tag tag;
        BudgetStatus status;        // Level::NoBudget se l'etichetta non ha tetto
        Currency budgetCurrency;    // valuta del tetto
        TagStats::Spending spending; // nel periodo e conto della dashboard
    };

    explicit TagSpendingView(QWidget *parent = nullptr);

    // `selected`: filtro attivo ("" = senza etichetta, nullopt = nessuno).
    void setItems(const QList<Item> &items, const TagStats::Spending &untagged, const Currency &accountCurrency,
                  const QString &periodName, const std::optional<QString> &selected);

signals:
    void tagClicked(const QString &tag); // vuoto = uscite senza etichetta
    void editRequested(qint64 tagId);

private:
    QVBoxLayout *m_layout;
};
