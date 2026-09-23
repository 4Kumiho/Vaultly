#pragma once

#include "core/Currency.h"
#include "core/Tag.h"
#include "ui/SidePanel.h"

#include <QList>

class ErrorLabel;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;
class SegmentedControl;

// Pannello laterale per creare, modificare o eliminare un'etichetta: nome, scadenza, tetto di spesa.
class TagPanel : public SidePanel
{
    Q_OBJECT

public:
    explicit TagPanel(QWidget *parent);

    // `currencies`: valute proposte per il tetto (quelle dei conti dell'utente).
    void openForCreate(qint64 userId, const QList<Currency> &currencies);
    void openForEdit(qint64 userId, const QList<Currency> &currencies, const Tag &tag);

signals:
    // Un'etichetta è stata salvata o eliminata.
    void changed();

private:
    void prepare(const QString &title, const QList<Currency> &currencies);
    TagPeriod currentPeriod() const;
    void updatePeriodHint();
    Currency selectedCurrency() const;
    void save();
    void deleteClicked();

    bool m_editing = false;
    bool m_confirmDelete = false;
    qint64 m_userId = 0;
    Tag m_tag;
    QList<Currency> m_currencies;

    QLabel *m_title;
    QLineEdit *m_name;
    SegmentedControl *m_period;
    QLabel *m_periodHint;
    QWidget *m_dates;
    QDateEdit *m_start;
    QDateEdit *m_end;
    QLineEdit *m_budget;
    QComboBox *m_currency;
    QLabel *m_budgetHint;
    ErrorLabel *m_error;
    QPushButton *m_delete;
};
