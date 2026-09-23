#include "platform/UpdateService.h"

#include "Version.h"
#include "core/Crypto.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

namespace {

constexpr int kFirstCheckDelayMs = 4000;
constexpr int kCheckIntervalMs = 24 * 60 * 60 * 1000;
const char *const kSkippedVersionKey = "updates/skippedVersion";

QNetworkRequest makeRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vaultly/%1").arg(VAULTLY_VERSION));
    // I redirect (GitHub → CDN) sono ammessi solo verso https.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(60000);
    return request;
}

QString tr(const char *text)
{
    return QCoreApplication::translate("UpdateService", text);
}

} // namespace

UpdateService::UpdateService(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    m_timer.setInterval(kCheckIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, [this] { check(false); });
}

bool UpdateService::isEnabled() const
{
    return VAULTLY_UPDATE_FEED_URL[0] != '\0';
}

QString UpdateService::currentVersion()
{
    return QStringLiteral(VAULTLY_VERSION);
}

void UpdateService::startAutomaticChecks()
{
    if (!isEnabled())
        return;
    QTimer::singleShot(kFirstCheckDelayMs, this, [this] { check(false); });
    m_timer.start();
}

void UpdateService::check(bool manual)
{
    if (!isEnabled() || m_checkReply || m_downloadReply)
        return;
    m_checkReply = m_network->get(makeRequest(QUrl(QStringLiteral(VAULTLY_UPDATE_FEED_URL))));
    QNetworkReply *reply = m_checkReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, manual] { onCheckFinished(reply, manual); });
}

void UpdateService::onCheckFinished(QNetworkReply *reply, bool manual)
{
    m_checkReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(tr("Impossibile contattare il server degli aggiornamenti. Controlla la connessione."));
        return;
    }
    const auto info = Appcast::parse(reply->readAll());
    if (!info) {
        emit checkFailed(tr("Le informazioni sull'aggiornamento non sono valide."));
        return;
    }
    if (Appcast::compareVersions(info->version, currentVersion()) <= 0) {
        emit upToDate();
        return;
    }
    const QString skipped = QSettings().value(kSkippedVersionKey).toString();
    if (!manual && info->version == skipped)
        return;

    m_update = *info;
    emit updateAvailable(m_update, manual);
}

void UpdateService::download()
{
    if (m_downloadReply || m_update.version.isEmpty())
        return;
    m_installerPath.clear();
    m_downloadReply = m_network->get(makeRequest(m_update.url));
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        // Mai più di quanto dichiarato nell'appcast: se succede, non è il nostro file.
        if (received > m_update.length) {
            cancelDownload();
            emit downloadFailed(tr("Il file scaricato non corrisponde all'aggiornamento atteso."));
            return;
        }
        emit downloadProgress(received, total > 0 ? total : m_update.length);
    });
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateService::onDownloadFinished);
}

void UpdateService::cancelDownload()
{
    if (!m_downloadReply)
        return;
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = nullptr;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
}

void UpdateService::onDownloadFinished()
{
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadFailed(tr("Download non riuscito. Controlla la connessione e riprova."));
        return;
    }
    const QByteArray installer = reply->readAll();

    // Dimensione e firma: senza entrambe corrette l'installer non si esegue.
    const QByteArray publicKey = QByteArray::fromBase64(VAULTLY_UPDATE_PUBLIC_KEY);
    if (installer.size() != m_update.length
        || !Crypto::verifyEd25519(publicKey, m_update.signature, installer)) {
        emit downloadFailed(tr("La firma dell'aggiornamento non è valida: il file è stato scartato."));
        return;
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Vaultly-update";
    QDir().mkpath(dir);
    const QString path = QStringLiteral("%1/Vaultly-Setup-%2.exe").arg(dir, m_update.version);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(installer) != installer.size()) {
        emit downloadFailed(tr("Impossibile salvare l'aggiornamento sul disco."));
        return;
    }
    file.close();
    m_installerPath = path;
    emit readyToInstall();
}

void UpdateService::skipCurrentUpdate()
{
    if (!m_update.version.isEmpty())
        QSettings().setValue(kSkippedVersionKey, m_update.version);
}

void UpdateService::installAndQuit()
{
    if (m_installerPath.isEmpty())
        return;
    // Stessi argomenti di sempre: /SILENT mostra solo l'avanzamento; a fine installazione l'app riparte.
    if (!QProcess::startDetached(m_installerPath, {"/SILENT", "/SP-", "/NOCANCEL"})) {
        emit downloadFailed(tr("Impossibile avviare l'installazione."));
        return;
    }
    QCoreApplication::quit();
}
