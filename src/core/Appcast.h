#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

#include <optional>

// Un aggiornamento descritto nell'appcast (feed RSS in formato Sparkle, generato da scripts/release.ps1).
struct UpdateInfo
{
    QString version;      // es. "1.0.3"
    QUrl url;             // installer da scaricare (solo https)
    QByteArray signature; // firma Ed25519 dell'installer, 64 byte
    qint64 length = 0;    // dimensione dell'installer in byte
    QString notesHtml;    // novità, in HTML semplice
};

namespace Appcast {

// La versione più recente per Windows presente nel feed, o nullopt se il feed non è valido.
// Le voci senza URL https o con firma/dimensione non valide vengono ignorate.
std::optional<UpdateInfo> parse(const QByteArray &xml);

// <0 se a < b, 0 se uguali, >0 se a > b. Confronto numerico per componenti: "1.0.10" > "1.0.9".
int compareVersions(const QString &a, const QString &b);

} // namespace Appcast
