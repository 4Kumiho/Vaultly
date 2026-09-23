#pragma once

#include "core/Currency.h"
#include "core/TagStats.h"

#include <QWidget>

#include <optional>

class QVBoxLayout;

// Elenco "quanto spendi per etichetta": nome, numero di spese, totale e barra proporzionale.
// In fondo le uscite senza etichetta. Cliccando una riga si filtra la lista dei movimenti.
class TagSpendingView : public QWidget
{
    Q_OBJECT

public:
    explicit TagSpendingView(QWidget *parent = nullptr);

    // `selected`: etichetta del filtro attivo ("" = senza etichetta, nullopt = nessun filtro).
    void setReport(const TagStats::Report &report, const Currency &currency, const std::optional<QString> &selected);

signals:
    // `tag` vuoto = le uscite senza etichetta.
    void tagClicked(const QString &tag);

private:
    QVBoxLayout *m_layout;
};
