#include "ui/UpdateOverlay.h"

#include "platform/UpdateService.h"
#include "ui/Components.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextDocument>
#include <QtMath>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int kCardWidth = 460;
constexpr int kBackdropAlpha = 160;
constexpr int kSlideDistance = 24;
constexpr int kMaxNotesHeight = 180;

QString megabytes(qint64 bytes)
{
    return QLocale().toString(double(bytes) / (1024.0 * 1024.0), 'f', 1) + " MB";
}
} // namespace

UpdateOverlay::UpdateOverlay(UpdateService *service, QWidget *host)
    : QWidget(host)
    , m_service(service)
    , m_anim(new QPropertyAnimation(this, "progress", this))
    , m_card(Components::card(this))
{
    hide();
    host->installEventFilter(this);
    setFocusPolicy(Qt::StrongFocus);

    m_anim->setDuration(300);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, [this] {
        if (m_progress <= 0.0)
            hide();
    });

    m_card->setFixedWidth(kCardWidth);

    m_icon = new QLabel(m_card);
    const qreal dpr = devicePixelRatioF();
    QPixmap icon = QPixmap(":/assets/vaultly.png").scaled(QSize(56, 56) * dpr, Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation);
    icon.setDevicePixelRatio(dpr);
    m_icon->setPixmap(icon);

    m_title = Components::label({}, "sectionTitle", m_card);
    m_title->setWordWrap(true);
    m_subtitle = Components::label({}, "muted", m_card);
    m_subtitle->setWordWrap(true);

    m_notes = new QTextBrowser(m_card);
    m_notes->setObjectName("releaseNotes");
    m_notes->setOpenLinks(false);

    m_bar = new QProgressBar(m_card);
    m_bar->setTextVisible(false);
    m_bar->setFixedHeight(8);
    m_barLabel = Components::label({}, "hint", m_card);

    m_primary = Components::button({}, "primary", m_card);
    m_secondary = Components::button({}, "secondary", m_card);
    m_skip = Components::button(tr("Salta questa versione"), "link", m_card);

    auto *header = new QHBoxLayout;
    header->setSpacing(16);
    header->addWidget(m_icon, 0, Qt::AlignTop);
    auto *titles = new QVBoxLayout;
    titles->setSpacing(4);
    titles->addWidget(m_title);
    titles->addWidget(m_subtitle);
    header->addLayout(titles, 1);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addWidget(m_skip);
    buttons->addStretch();
    buttons->addWidget(m_secondary);
    buttons->addWidget(m_primary);

    auto *layout = new QVBoxLayout(m_card);
    layout->setContentsMargins(28, 26, 28, 24);
    layout->setSpacing(14);
    layout->addLayout(header);
    layout->addWidget(m_notes);
    layout->addWidget(m_bar);
    layout->addWidget(m_barLabel);
    layout->addSpacing(4);
    layout->addLayout(buttons);

    // Azioni dei pulsanti in base allo stato.
    connect(m_primary, &QPushButton::clicked, this, [this] {
        switch (m_state) {
        case State::Available:
            setState(State::Downloading);
            m_service->download();
            break;
        case State::Failed:
            if (m_failedWhileDownloading) {
                setState(State::Downloading);
                m_service->download();
            } else {
                setState(State::Checking);
                m_service->check(true);
            }
            break;
        default:
            hideAnimated();
        }
    });
    connect(m_secondary, &QPushButton::clicked, this, [this] {
        if (m_state == State::Downloading)
            m_service->cancelDownload();
        hideAnimated();
    });
    connect(m_skip, &QPushButton::clicked, this, [this] {
        m_service->skipCurrentUpdate();
        hideAnimated();
    });

    // Eventi del servizio. I controlli automatici lavorano in silenzio: la schermata compare
    // solo se trovano un aggiornamento; "aggiornato" ed errori si mostrano solo al controllo manuale.
    connect(m_service, &UpdateService::updateAvailable, this, [this](const UpdateInfo &update, bool manual) {
        if (isVisible() && m_state != State::Checking)
            return;
        m_update = update;
        m_manual = manual;
        setState(State::Available);
        showAnimated();
    });
    connect(m_service, &UpdateService::upToDate, this, [this] {
        if (isVisible() && m_state == State::Checking)
            setState(State::UpToDate);
    });
    connect(m_service, &UpdateService::checkFailed, this, [this](const QString &message) {
        if (!isVisible() || m_state != State::Checking)
            return;
        m_failedWhileDownloading = false;
        m_subtitle->setText(message);
        setState(State::Failed);
    });
    connect(m_service, &UpdateService::downloadProgress, this, [this](qint64 received, qint64 total) {
        m_bar->setRange(0, 1000);
        m_bar->setValue(total > 0 ? int(received * 1000 / total) : 0);
        m_barLabel->setText(tr("%1 di %2").arg(megabytes(received), megabytes(total)));
    });
    connect(m_service, &UpdateService::downloadFailed, this, [this](const QString &message) {
        m_failedWhileDownloading = true;
        m_subtitle->setText(message);
        setState(State::Failed);
    });
    connect(m_service, &UpdateService::readyToInstall, this, [this] {
        setState(State::Installing);
        // Un attimo per leggere il messaggio, poi l'installer prende il posto dell'app.
        QTimer::singleShot(900, m_service, &UpdateService::installAndQuit);
    });
}

void UpdateOverlay::checkNow()
{
    m_manual = true;
    setState(State::Checking);
    showAnimated();
    m_service->check(true);
}

void UpdateOverlay::setState(State state)
{
    m_state = state;
    const UpdateInfo &u = m_update;
    const QString current = UpdateService::currentVersion();

    m_notes->hide();
    m_bar->hide();
    m_barLabel->hide();
    m_skip->hide();
    m_secondary->show();
    m_primary->show();
    m_primary->setEnabled(true);
    m_secondary->setEnabled(true);

    switch (state) {
    case State::Checking:
        m_title->setText(tr("Controllo aggiornamenti…"));
        m_subtitle->setText(tr("Versione installata: %1").arg(current));
        m_bar->setRange(0, 0); // indeterminata
        m_bar->show();
        m_primary->hide();
        m_secondary->setText(tr("Chiudi"));
        break;
    case State::UpToDate:
        m_title->setText(tr("Vaultly è aggiornato"));
        m_subtitle->setText(tr("Hai già l'ultima versione (%1).").arg(current));
        m_primary->setText(tr("Ok"));
        m_secondary->hide();
        break;
    case State::Available:
        m_title->setText(tr("È disponibile Vaultly %1").arg(u.version));
        m_subtitle->setText(tr("Hai la versione %1. L'aggiornamento richiede meno di un minuto e i tuoi dati "
                               "restano dove sono.")
                                .arg(current));
        if (!u.notesHtml.isEmpty()) {
            m_notes->setHtml(u.notesHtml);
            // Alto quanto il testo (entro kMaxNotesHeight, poi scorre).
            QTextDocument *doc = m_notes->document();
            doc->setIndentWidth(14);
            doc->setTextWidth(kCardWidth - 2 * 28 - 24);
            const int frame = 2 * m_notes->frameWidth() + 12;
            m_notes->setFixedHeight(std::min(qCeil(doc->size().height()) + frame, kMaxNotesHeight));
            m_notes->show();
        }
        m_primary->setText(tr("Aggiorna ora"));
        m_secondary->setText(tr("Più tardi"));
        m_skip->setVisible(!m_manual);
        break;
    case State::Downloading:
        m_title->setText(tr("Download di Vaultly %1").arg(u.version));
        m_subtitle->setText(tr("Al termine il file viene verificato con la firma digitale di Vaultly."));
        m_bar->setRange(0, 1000);
        m_bar->setValue(0);
        m_bar->show();
        m_barLabel->setText(tr("0 MB di %1").arg(megabytes(u.length)));
        m_barLabel->show();
        m_primary->hide();
        m_secondary->setText(tr("Annulla"));
        break;
    case State::Installing:
        m_title->setText(tr("Installazione…"));
        m_subtitle->setText(tr("Firma verificata. Vaultly si chiude e si riapre da solo tra un attimo."));
        m_bar->setRange(0, 0);
        m_bar->show();
        m_primary->hide();
        m_secondary->hide();
        break;
    case State::Failed:
        m_title->setText(tr("Aggiornamento non riuscito"));
        // Il messaggio d'errore è già nel sottotitolo.
        m_primary->setText(tr("Riprova"));
        m_secondary->setText(tr("Chiudi"));
        break;
    }
    m_card->adjustSize();
    layoutCard();
}

void UpdateOverlay::showAnimated()
{
    setGeometry(parentWidget()->rect());
    show();
    raise();
    setFocus();
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(1.0);
    m_anim->start();
}

void UpdateOverlay::hideAnimated()
{
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(0.0);
    m_anim->start();
}

void UpdateOverlay::setProgress(qreal progress)
{
    m_progress = progress;
    layoutCard();
    update();
}

void UpdateOverlay::layoutCard()
{
    m_card->adjustSize();
    const int x = (width() - m_card->width()) / 2;
    const int y = (height() - m_card->height()) / 2 + qRound((1.0 - m_progress) * kSlideDistance);
    m_card->move(x, y);
}

bool UpdateOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        layoutCard();
    }
    return QWidget::eventFilter(watched, event);
}

void UpdateOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, qRound(kBackdropAlpha * m_progress)));
}

void UpdateOverlay::keyPressEvent(QKeyEvent *event)
{
    // Esc chiude, tranne durante l'installazione.
    if (event->key() == Qt::Key_Escape && m_state != State::Installing) {
        if (m_state == State::Downloading)
            m_service->cancelDownload();
        hideAnimated();
        return;
    }
    QWidget::keyPressEvent(event);
}
