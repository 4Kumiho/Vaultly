#include "core/VaultService.h"

#include "core/Crypto.h"
#include "db/VaultRepository.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>

#include <algorithm>

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("VaultService", text);
}

VaultService::Result failure(const QString &message)
{
    return {std::nullopt, message};
}

QByteArray serialize(const VaultEntry &e)
{
    const QJsonObject json{
        {"title", e.title},
        {"url", e.url},
        {"username", e.username},
        {"password", e.password},
        {"notes", e.notes},
    };
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

std::optional<VaultEntry> deserialize(const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return std::nullopt;
    const QJsonObject json = doc.object();
    VaultEntry e;
    e.title = json.value("title").toString();
    e.url = json.value("url").toString();
    e.username = json.value("username").toString();
    e.password = json.value("password").toString();
    e.notes = json.value("notes").toString();
    return e;
}

// Pulisce i campi e restituisce un messaggio d'errore, o una stringa vuota se la voce va bene.
QString normalize(VaultEntry &e)
{
    e.title = e.title.trimmed();
    e.url = e.url.trimmed();
    e.username = e.username.trimmed();
    e.notes = e.notes.trimmed();
    if (e.title.isEmpty())
        return tr("Inserisci un nome, es. \"Gmail\".");
    if (e.username.isEmpty() && e.password.isEmpty())
        return tr("Inserisci almeno lo username o la password.");
    return {};
}

std::optional<QByteArray> seal(const Session &session, const VaultEntry &e)
{
    return Crypto::encrypt(session.vaultKey, serialize(e));
}

} // namespace

QList<VaultEntry> VaultService::list(const Session &session, int *unreadable)
{
    QList<VaultEntry> result;
    int failed = 0;
    for (const auto &row : VaultRepository::listForUser(session.user.id)) {
        const auto plain = Crypto::decrypt(session.vaultKey, row.data);
        auto entry = plain ? deserialize(*plain) : std::nullopt;
        if (!entry) {
            ++failed;
            continue;
        }
        entry->id = row.id;
        entry->updatedAt = row.updatedAt;
        result.append(*entry);
    }
    std::sort(result.begin(), result.end(), [](const VaultEntry &a, const VaultEntry &b) {
        return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
    });
    if (unreadable)
        *unreadable = failed;
    return result;
}

VaultService::Result VaultService::create(const Session &session, const VaultEntry &entry)
{
    VaultEntry e = entry;
    if (const QString error = normalize(e); !error.isEmpty())
        return failure(error);

    const auto blob = seal(session, e);
    const auto id = blob ? VaultRepository::insert(session.user.id, *blob) : std::nullopt;
    if (!id)
        return failure(tr("Impossibile salvare la voce."));
    e.id = *id;
    e.updatedAt = QDateTime::currentDateTime();
    return {e, {}};
}

VaultService::Result VaultService::update(const Session &session, const VaultEntry &entry)
{
    VaultEntry e = entry;
    if (const QString error = normalize(e); !error.isEmpty())
        return failure(error);

    const auto blob = seal(session, e);
    if (!blob || !VaultRepository::update(e.id, session.user.id, *blob))
        return failure(tr("Impossibile salvare la voce."));
    e.updatedAt = QDateTime::currentDateTime();
    return {e, {}};
}

bool VaultService::remove(const Session &session, qint64 entryId)
{
    return VaultRepository::remove(entryId, session.user.id);
}

QString VaultService::generatePassword(int length, bool symbols)
{
    static const QString lower = "abcdefghijkmnopqrstuvwxyz"; // senza la 'l', che si confonde con 1
    static const QString upper = "ABCDEFGHJKLMNPQRSTUVWXYZ";  // senza I e O
    static const QString digits = "23456789";                 // senza 0 e 1
    static const QString symbolChars = "!@#$%&*-_=+?";

    QList<QString> classes = {lower, upper, digits};
    if (symbols)
        classes.append(symbolChars);
    const QString all = classes.join(QString());
    length = std::max<int>(length, int(classes.size()));

    // QRandomGenerator::system() usa il generatore crittografico del sistema operativo.
    QRandomGenerator *rng = QRandomGenerator::system();
    for (;;) {
        QString password;
        for (int i = 0; i < length; ++i)
            password.append(all.at(rng->bounded(int(all.size()))));

        const bool hasEveryClass = std::all_of(classes.cbegin(), classes.cend(), [&](const QString &chars) {
            return std::any_of(password.cbegin(), password.cend(), [&](QChar c) { return chars.contains(c); });
        });
        if (hasEveryClass)
            return password;
    }
}
