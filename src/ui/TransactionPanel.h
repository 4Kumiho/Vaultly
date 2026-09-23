#pragma once

#include "core/Account.h"
#include "core/Transaction.h"
#include "ui/SidePanel.h"

#include <QList>

class ErrorLabel;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class SegmentedControl;

// Pannello laterale per aggiungere, modificare o eliminare un movimento.
class TransactionPanel : public SidePanel
{
    Q_OBJECT

public:
    explicit TransactionPanel(QWidget *parent);

    void openForCreate(qint64 userId, const Account &account);
    void openForEdit(qint64 userId, const Account &account, const Transaction &transaction);

signals:
    // Un movimento è stato salvato o eliminato.
    void changed();

private:
    void prepare(const QString &title);
    TransactionType currentType() const;
    void setType(TransactionType type, qint64 selectCategoryId = 0);
    void save();
    void deleteClicked();

    bool m_editing = false;
    bool m_confirmDelete = false;
    qint64 m_userId = 0;
    Account m_account;
    Transaction m_transaction;
    QList<Category> m_categories;

    QLabel *m_title;
    QLabel *m_subtitle;
    SegmentedControl *m_type;
    QLineEdit *m_amount;
    QComboBox *m_category;
    QDateTimeEdit *m_date;
    QLineEdit *m_description;
    ErrorLabel *m_error;
    QPushButton *m_delete;
};
