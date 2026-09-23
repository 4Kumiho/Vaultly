#pragma once

#include <QFrame>
#include <QStringList>

class FlowLayout;
class QCompleter;
class QLineEdit;
class QStringListModel;

// Campo per le etichette: si scrive, Invio (o virgola) crea un chip; la × lo toglie,
// Backspace a campo vuoto toglie l'ultimo. Suggerisce le etichette già usate.
class TagInput : public QFrame
{
    Q_OBJECT

public:
    explicit TagInput(QWidget *parent = nullptr);

    void setTags(const QStringList &tags);
    // Chip + testo ancora da confermare (così non si perde se si salva senza premere Invio).
    QStringList tags() const;
    void setSuggestions(const QStringList &suggestions);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void addTag(QString tag);
    void removeTag(const QString &tag);
    void commitText();
    void rebuild();

    QStringList m_tags;
    FlowLayout *m_flow;
    QLineEdit *m_edit;
    QStringListModel *m_model;
    QCompleter *m_completer;
};
