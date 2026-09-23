#pragma once

#include "core/Appcast.h"

#include <QObject>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

// Aggiornamenti automatici.
// 1. legge l'appcast pubblicato con l'ultima release (VAULTLY_UPDATE_FEED_URL);
// 2. se c'è una versione più nuova lo segnala (la UI è UpdateOverlay);
// 3. su richiesta scarica l'installer, ne verifica dimensione e firma Ed25519
//    con la chiave pubblica compilata nell'app, lo lancia in modalità /SILENT ed esce.
// L'installer riavvia l'app a fine installazione.
class UpdateService : public QObject
{
    Q_OBJECT

public:
    explicit UpdateService(QObject *parent = nullptr);

    // Falso nelle build di sviluppo senza VAULTLY_GITHUB_REPO.
    bool isEnabled() const;
    static QString currentVersion();

    // Controllo all'avvio (dopo qualche secondo) e poi ogni 24 ore.
    void startAutomaticChecks();

    // `manual`: richiesto dall'utente, quindi si segnala anche "nessun aggiornamento"
    // e si ignora la versione saltata.
    void check(bool manual);

    void download();
    void cancelDownload();
    void skipCurrentUpdate();

    // Lancia l'installer verificato e chiude l'app.
    void installAndQuit();

signals:
    void updateAvailable(const UpdateInfo &update, bool manual);
    void upToDate();
    void checkFailed(const QString &message);

    void downloadProgress(qint64 received, qint64 total);
    void downloadFailed(const QString &message);
    void readyToInstall();

private:
    void onCheckFinished(QNetworkReply *reply, bool manual);
    void onDownloadFinished();

    QNetworkAccessManager *m_network;
    QTimer m_timer;
    QNetworkReply *m_checkReply = nullptr;
    QNetworkReply *m_downloadReply = nullptr;
    UpdateInfo m_update;
    QString m_installerPath;
};
