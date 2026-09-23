#include "ui/VaultPage.h"

#include "core/VaultService.h"
#include "ui/Animations.h"
#include "ui/Components.h"
#include "ui/VaultPanel.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

constexpr int kClipboardClearMs = 30000;
constexpr int kAnimatedRows = 25;

// Riga di una voce: iniziale, nome e username, pulsanti per copiare.
class VaultRow : public ClickableFrame
{
public:
    QPushButton *copyUser;
    QPushButton *copyPassword;

    VaultRow(const VaultEntry &e, QWidget *parent)
        : ClickableFrame(parent)
    {
        setObjectName("listRow");

        auto *icon = new QLabel(e.title.left(1).toUpper(), this);
        icon->setObjectName("rowIcon");
        icon->setProperty("tone", "accent");
        icon->setFixedSize(38, 38);
        icon->setAlignment(Qt::AlignCenter);

        auto *title = Components::label(e.title, "rowTitle", this);
        title->setTextFormat(Qt::PlainText);
        const QString host = QUrl::fromUserInput(e.url).host();
        QString detail = e.username.isEmpty() ? QString("—") : e.username;
        if (!host.isEmpty())
            detail += QString("  ·  %1").arg(host);
        auto *subtitle = Components::label(detail, "muted", this);
        subtitle->setTextFormat(Qt::PlainText);

        copyUser = Components::button(tr("Copia utente"), "chip", this);
        copyPassword = Components::button(tr("Copia password"), "chip", this);
        copyUser->setEnabled(!e.username.isEmpty());
        copyPassword->setEnabled(!e.password.isEmpty());
        for (QPushButton *b : {copyUser, copyPassword})
            b->setFocusPolicy(Qt::NoFocus);

        auto *texts = new QVBoxLayout;
        texts->setSpacing(1);
        texts->addWidget(title);
        texts->addWidget(subtitle);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(14);
        layout->addWidget(icon);
        layout->addLayout(texts, 1);
        layout->addWidget(copyUser);
        layout->addWidget(copyPassword);

        for (QLabel *l : {icon, title, subtitle})
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    static QString tr(const char *text) { return QCoreApplication::translate("VaultPage", text); }
};

bool matches(const VaultEntry &e, const QString &query)
{
    if (query.isEmpty())
        return true;
    for (const QString &field : {e.title, e.username, e.url}) {
        if (field.contains(query, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

} // namespace

VaultPage::VaultPage(QWidget *overlayHost, QWidget *parent)
    : QWidget(parent)
    , m_clipboardTimer(new QTimer(this))
{
    // Intestazione della sezione.
    auto *title = Components::label(tr("Le tue password"), "sectionTitle", this);
    auto *subtitle = Components::label(tr("Cifrate con AES-256 usando la tua password di accesso. "
                                          "Se la dimentichi, non si possono recuperare."),
                                       "muted", this);
    subtitle->setWordWrap(true);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Cerca per nome, utente o sito"));
    m_search->setClearButtonEnabled(true);
    m_search->setFixedWidth(280);
    auto *addButton = Components::button(tr("+  Nuova password"), "primary", this);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addWidget(title);
    titles->addWidget(subtitle);
    auto *top = new QHBoxLayout;
    top->setSpacing(12);
    top->addLayout(titles, 1);
    top->addWidget(m_search, 0, Qt::AlignVCenter);
    top->addWidget(addButton, 0, Qt::AlignVCenter);

    // Elenco.
    m_listCard = Components::card(this);
    m_count = Components::label({}, "caption", m_listCard);
    auto *listLayout = new QVBoxLayout(m_listCard);
    listLayout->setContentsMargins(20, 18, 20, 18);
    listLayout->setSpacing(10);
    listLayout->addWidget(m_count);
    m_listLayout = new QVBoxLayout;
    m_listLayout->setSpacing(2);
    listLayout->addLayout(m_listLayout);

    // Stato vuoto.
    m_emptyState = Components::card(this);
    auto *createFirst = Components::button(tr("Salva la tua prima password"), "primary", m_emptyState);
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(32, 40, 32, 40);
    emptyLayout->setSpacing(8);
    emptyLayout->addWidget(Components::label(tr("Nessuna password salvata"), "title", m_emptyState), 0,
                           Qt::AlignHCenter);
    emptyLayout->addWidget(Components::label(tr("Tieni qui login e password dei tuoi account, al sicuro."),
                                             "subtitle", m_emptyState),
                           0, Qt::AlignHCenter);
    emptyLayout->addSpacing(16);
    emptyLayout->addWidget(createFirst, 0, Qt::AlignHCenter);

    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(44, 16, 44, 36);
    layout->setSpacing(18);
    layout->addLayout(top);
    layout->addWidget(m_listCard);
    layout->addWidget(m_emptyState);
    layout->addStretch(); // aggiungendoli al layout, i widget passano sotto `content`

    auto *scroll = new QScrollArea(this);
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scroll);

    m_panel = new VaultPanel(overlayHost);

    m_clipboardTimer->setSingleShot(true);
    connect(m_clipboardTimer, &QTimer::timeout, this, &VaultPage::clearClipboardIfOurs);
    connect(m_search, &QLineEdit::textChanged, this, [this] { rebuildList(false); });
    connect(addButton, &QPushButton::clicked, this, [this] { m_panel->openForCreate(m_session); });
    connect(createFirst, &QPushButton::clicked, this, [this] { m_panel->openForCreate(m_session); });
    connect(m_panel, &VaultPanel::changed, this, [this](const QString &message) {
        reload(false);
        emit notify(message);
    });
}

void VaultPage::setSession(const Session &session)
{
    m_session = session;
    m_search->clear();
    reload();
}

void VaultPage::clearSession()
{
    clearClipboardIfOurs();
    m_session.vaultKey.fill('\0');
    m_session = {};
    m_entries.clear();
    rebuildList(false);
}

void VaultPage::dismissPanels()
{
    m_panel->dismiss(false);
}

void VaultPage::reload(bool animate)
{
    int unreadable = 0;
    m_entries = m_session.vaultKey.isEmpty() ? QList<VaultEntry>() : VaultService::list(m_session, &unreadable);
    rebuildList(animate);
    if (unreadable > 0)
        emit notify(tr("%1 voci non leggibili: i dati potrebbero essere danneggiati.").arg(unreadable));
}

void VaultPage::rebuildList(bool animate)
{
    while (QLayoutItem *item = m_listLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    const QString query = m_search->text().trimmed();
    int shown = 0;
    for (const VaultEntry &e : std::as_const(m_entries)) {
        if (!matches(e, query))
            continue;
        auto *row = new VaultRow(e, m_listCard);
        connect(row, &ClickableFrame::clicked, this, [this, e] { m_panel->openForEdit(m_session, e); });
        connect(row->copyUser, &QPushButton::clicked, this,
                [this, e] { copyToClipboard(e.username, tr("Username copiato")); });
        connect(row->copyPassword, &QPushButton::clicked, this,
                [this, e] { copyToClipboard(e.password, tr("Password copiata")); });
        m_listLayout->addWidget(row);
        if (animate && shown < kAnimatedRows)
            Animations::fadeIn(row, 260, shown * 30);
        ++shown;
    }

    if (shown == 0 && !m_entries.isEmpty()) {
        auto *none = Components::label(tr("Nessun risultato per \"%1\".").arg(query), "muted", m_listCard);
        none->setTextFormat(Qt::PlainText);
        none->setAlignment(Qt::AlignCenter);
        none->setMinimumHeight(70);
        m_listLayout->addWidget(none);
    }

    m_count->setText(query.isEmpty() ? tr("%1 VOCI").arg(m_entries.size())
                                     : tr("%1 DI %2 VOCI").arg(shown).arg(m_entries.size()));
    m_listCard->setVisible(!m_entries.isEmpty());
    m_emptyState->setVisible(m_entries.isEmpty());
}

void VaultPage::copyToClipboard(const QString &text, const QString &message)
{
    auto *mime = new QMimeData;
    mime->setText(text);
    // Windows: niente cronologia degli appunti (Win+V) e niente sincronizzazione cloud.
    const QByteArray zero(4, '\0');
    mime->setData(R"(application/x-qt-windows-mime;value="ExcludeClipboardContentFromMonitorProcessing")", zero);
    mime->setData(R"(application/x-qt-windows-mime;value="CanIncludeInClipboardHistory")", zero);
    mime->setData(R"(application/x-qt-windows-mime;value="CanUploadToCloudClipboard")", zero);
    QGuiApplication::clipboard()->setMimeData(mime);

    m_copiedText = text;
    m_clipboardTimer->start(kClipboardClearMs);
    emit notify(tr("%1 · gli appunti si svuotano tra 30 secondi").arg(message));
}

void VaultPage::clearClipboardIfOurs()
{
    m_clipboardTimer->stop();
    // Si svuota solo se negli appunti c'è ancora quello che abbiamo copiato noi.
    if (!m_copiedText.isEmpty() && QGuiApplication::clipboard()->text() == m_copiedText)
        QGuiApplication::clipboard()->clear();
    m_copiedText.clear();
}
