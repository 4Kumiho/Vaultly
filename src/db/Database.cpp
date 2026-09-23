#include "db/Database.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

constexpr int kSchemaVersion = 3;
// Versione più vecchia che importUsersFrom sa leggere (i DB della 1.0.3 sono v2).
constexpr int kOldestImportableVersion = 2;

const char *const kSchemaV1[] = {
    R"(CREATE TABLE users (
        id            INTEGER PRIMARY KEY AUTOINCREMENT,
        username      TEXT    NOT NULL UNIQUE COLLATE NOCASE,
        password_hash BLOB    NOT NULL,
        salt          BLOB    NOT NULL,
        iterations    INTEGER NOT NULL,
        created_at    TEXT    NOT NULL
    ))",
    R"(CREATE TABLE currencies (
        code        TEXT    PRIMARY KEY,
        symbol      TEXT    NOT NULL,
        minor_units INTEGER NOT NULL
    ))",
    R"(CREATE TABLE accounts (
        id              INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id         INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        name            TEXT    NOT NULL,
        currency_code   TEXT    NOT NULL REFERENCES currencies(code),
        initial_balance INTEGER NOT NULL,
        created_at      TEXT    NOT NULL,
        UNIQUE (user_id, name)
    ))",
    R"(CREATE TABLE categories (
        id   INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT    NOT NULL,
        type TEXT    NOT NULL CHECK (type IN ('income', 'expense')),
        UNIQUE (name, type)
    ))",
    R"(CREATE TABLE transactions (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        account_id  INTEGER NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
        category_id INTEGER NOT NULL REFERENCES categories(id),
        amount      INTEGER NOT NULL CHECK (amount > 0),
        type        TEXT    NOT NULL CHECK (type IN ('income', 'expense')),
        description TEXT,
        occurred_at TEXT    NOT NULL
    ))",
    "CREATE INDEX idx_tx_account_date ON transactions(account_id, occurred_at)",
};

// v2: area password. Ogni voce è un unico blob cifrato (AES-256-GCM) con una chiave
// derivata dalla password di login e da vault_salt; nel DB non resta nulla in chiaro.
const char *const kSchemaV2[] = {
    "ALTER TABLE users ADD COLUMN vault_salt BLOB",
    R"(CREATE TABLE vault_entries (
        id         INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id    INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        data       BLOB    NOT NULL,
        created_at TEXT    NOT NULL,
        updated_at TEXT    NOT NULL
    ))",
    "CREATE INDEX idx_vault_user ON vault_entries(user_id)",
};

// v3: etichette libere dei movimenti. Ogni utente ha le sue; nomi unici senza distinguere
// maiuscole. Un'etichetta senza più movimenti viene eliminata (TagRepository::deleteUnused).
const char *const kSchemaV3[] = {
    R"(CREATE TABLE tags (
        id      INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        name    TEXT    NOT NULL COLLATE NOCASE,
        UNIQUE (user_id, name)
    ))",
    R"(CREATE TABLE transaction_tags (
        transaction_id INTEGER NOT NULL REFERENCES transactions(id) ON DELETE CASCADE,
        tag_id         INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE,
        PRIMARY KEY (transaction_id, tag_id)
    ))",
    "CREATE INDEX idx_txtags_tag ON transaction_tags(tag_id)",
};

struct CurrencySeed
{
    const char *code;
    const char *symbol;
    int minorUnits;
};

const CurrencySeed kCurrencies[] = {
    {"EUR", "€", 2},
    {"USD", "$", 2},
    {"GBP", "£", 2},
    {"CHF", "CHF", 2},
    {"JPY", "¥", 0},
};

struct CategorySeed
{
    const char *name;
    const char *type;
};

const CategorySeed kCategories[] = {
    {"Stipendio", "income"},
    {"Regali", "income"},
    {"Rimborsi", "income"},
    {"Investimenti", "income"},
    {"Altro", "income"},
    {"Casa", "expense"},
    {"Spesa", "expense"},
    {"Trasporti", "expense"},
    {"Bollette", "expense"},
    {"Salute", "expense"},
    {"Svago", "expense"},
    {"Ristoranti", "expense"},
    {"Abbonamenti", "expense"},
    {"Altro", "expense"},
};

bool fail(const QSqlError &sqlError, QString *error)
{
    if (error)
        *error = sqlError.text();
    return false;
}

bool createSchemaV1(QSqlDatabase &db, QString *error)
{
    QSqlQuery q(db);
    for (const char *sql : kSchemaV1) {
        if (!q.exec(QString::fromUtf8(sql)))
            return fail(q.lastError(), error);
    }

    if (!q.prepare("INSERT INTO currencies (code, symbol, minor_units) VALUES (?, ?, ?)"))
        return fail(q.lastError(), error);
    for (const auto &c : kCurrencies) {
        q.addBindValue(QString::fromUtf8(c.code));
        q.addBindValue(QString::fromUtf8(c.symbol));
        q.addBindValue(c.minorUnits);
        if (!q.exec())
            return fail(q.lastError(), error);
    }

    if (!q.prepare("INSERT INTO categories (name, type) VALUES (?, ?)"))
        return fail(q.lastError(), error);
    for (const auto &c : kCategories) {
        q.addBindValue(QString::fromUtf8(c.name));
        q.addBindValue(QString::fromUtf8(c.type));
        if (!q.exec())
            return fail(q.lastError(), error);
    }
    return true;
}

template <size_t N>
bool execAll(QSqlDatabase &db, const char *const (&statements)[N], QString *error)
{
    QSqlQuery q(db);
    for (const char *sql : statements) {
        if (!q.exec(QString::fromUtf8(sql)))
            return fail(q.lastError(), error);
    }
    return true;
}

bool createSchemaV2(QSqlDatabase &db, QString *error)
{
    return execAll(db, kSchemaV2, error);
}

bool createSchemaV3(QSqlDatabase &db, QString *error)
{
    return execAll(db, kSchemaV3, error);
}

} // namespace

bool Database::open(const QString &path, QString *error)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open())
        return fail(db.lastError(), error);

    QSqlQuery q(db);
    // SQLite tiene le foreign key spente di default, va attivato a ogni connessione.
    if (!q.exec("PRAGMA foreign_keys = ON"))
        return fail(q.lastError(), error);
    if (!q.exec("PRAGMA user_version") || !q.next())
        return fail(q.lastError(), error);
    const int version = q.value(0).toInt();

    if (version >= kSchemaVersion)
        return true;

    // Migrazioni in ordine, tutte nella stessa transazione.
    db.transaction();
    if ((version < 1 && !createSchemaV1(db, error)) || (version < 2 && !createSchemaV2(db, error))
        || (version < 3 && !createSchemaV3(db, error))) {
        db.rollback();
        return false;
    }
    if (!q.exec(QString("PRAGMA user_version = %1").arg(kSchemaVersion))) {
        db.rollback();
        return fail(q.lastError(), error);
    }
    if (!db.commit())
        return fail(db.lastError(), error);
    return true;
}

namespace {

// Esegue una query preparata con i valori dati; false (ed errore) se fallisce.
bool run(QSqlQuery &q, const QString &sql, const QVariantList &values, QString *error)
{
    if (!q.prepare(sql))
        return fail(q.lastError(), error);
    for (const QVariant &v : values)
        q.addBindValue(v);
    if (!q.exec())
        return fail(q.lastError(), error);
    return true;
}

QList<qint64> ids(QSqlDatabase &db, const QString &sql, const QVariantList &values, QString *error, bool *ok)
{
    QList<qint64> result;
    QSqlQuery q(db);
    *ok = run(q, sql, values, error);
    while (*ok && q.next())
        result.append(q.value(0).toLongLong());
    return result;
}

// Copia utenti, conti, movimenti (con le etichette, se `otherHasTags`) e voci dell'area password
// dal DB "other" (già ATTACH). Gli id cambiano; le categorie si abbinano per nome e tipo.
bool copyUsers(QSqlDatabase &db, bool otherHasTags, int *imported, QString *error)
{
    bool ok = true;
    const auto userIds = ids(db,
                             "SELECT id FROM other.users "
                             "WHERE username NOT IN (SELECT username FROM main.users)",
                             {}, error, &ok);
    if (!ok)
        return false;

    QSqlQuery q(db);
    for (const qint64 oldUser : userIds) {
        if (!run(q,
                 "INSERT INTO main.users (username, password_hash, salt, iterations, created_at, vault_salt) "
                 "SELECT username, password_hash, salt, iterations, created_at, vault_salt "
                 "FROM other.users WHERE id = ?",
                 {oldUser}, error))
            return false;
        const qint64 newUser = q.lastInsertId().toLongLong();

        // Etichette: vecchio id → nuovo id.
        QHash<qint64, qint64> tagMap;
        if (otherHasTags) {
            const auto tagIds = ids(db, "SELECT id FROM other.tags WHERE user_id = ?", {oldUser}, error, &ok);
            if (!ok)
                return false;
            for (const qint64 oldTag : tagIds) {
                if (!run(q, "INSERT INTO main.tags (user_id, name) SELECT ?, name FROM other.tags WHERE id = ?",
                         {newUser, oldTag}, error))
                    return false;
                tagMap.insert(oldTag, q.lastInsertId().toLongLong());
            }
        }

        const auto accountIds = ids(db, "SELECT id FROM other.accounts WHERE user_id = ?", {oldUser}, error, &ok);
        if (!ok)
            return false;
        for (const qint64 oldAccount : accountIds) {
            if (!run(q,
                     "INSERT INTO main.accounts (user_id, name, currency_code, initial_balance, created_at) "
                     "SELECT ?, name, currency_code, initial_balance, created_at FROM other.accounts WHERE id = ?",
                     {newUser, oldAccount}, error))
                return false;
            const qint64 newAccount = q.lastInsertId().toLongLong();

            // Un movimento alla volta, per poter ricollegare le sue etichette.
            const auto txIds =
                ids(db, "SELECT id FROM other.transactions WHERE account_id = ?", {oldAccount}, error, &ok);
            if (!ok)
                return false;
            for (const qint64 oldTx : txIds) {
                if (!run(q,
                         "INSERT INTO main.transactions "
                         "(account_id, category_id, amount, type, description, occurred_at) "
                         "SELECT ?, (SELECT m.id FROM main.categories m JOIN other.categories o "
                         "           ON o.name = m.name AND o.type = m.type WHERE o.id = t.category_id), "
                         "       t.amount, t.type, t.description, t.occurred_at "
                         "FROM other.transactions t WHERE t.id = ?",
                         {newAccount, oldTx}, error))
                    return false;
                const qint64 newTx = q.lastInsertId().toLongLong();
                if (!otherHasTags)
                    continue;
                const auto txTags =
                    ids(db, "SELECT tag_id FROM other.transaction_tags WHERE transaction_id = ?", {oldTx}, error, &ok);
                if (!ok)
                    return false;
                for (const qint64 oldTag : txTags) {
                    if (!run(q, "INSERT INTO main.transaction_tags (transaction_id, tag_id) VALUES (?, ?)",
                             {newTx, tagMap.value(oldTag)}, error))
                        return false;
                }
            }
        }

        // Le voci restano cifrate: chiave = password dell'utente + vault_salt, copiato sopra.
        if (!run(q,
                 "INSERT INTO main.vault_entries (user_id, data, created_at, updated_at) "
                 "SELECT ?, data, created_at, updated_at FROM other.vault_entries WHERE user_id = ?",
                 {newUser, oldUser}, error))
            return false;
        ++*imported;
    }
    return true;
}

} // namespace

std::optional<int> Database::importUsersFrom(const QString &otherPath, int *conflicts, QString *error)
{
    QSqlDatabase db = QSqlDatabase::database();
    {
        QSqlQuery q(db);
        if (!run(q, "ATTACH DATABASE ? AS other", {otherPath}, error))
            return std::nullopt;
    }

    std::optional<int> result;
    {
        QSqlQuery q(db);
        const int otherVersion = q.exec("PRAGMA other.user_version") && q.next() ? q.value(0).toInt() : 0;
        q.finish();
        if (otherVersion < kOldestImportableVersion || otherVersion > kSchemaVersion) {
            if (error)
                *error = QStringLiteral("Versione del database da importare non supportata.");
        } else if (q.exec("SELECT COUNT(*) FROM other.users WHERE username IN (SELECT username FROM main.users)")
                   && q.next()) {
            if (conflicts)
                *conflicts = q.value(0).toInt();
            q.finish();

            int imported = 0;
            db.transaction();
            if (copyUsers(db, otherVersion >= 3, &imported, error) && db.commit())
                result = imported;
            else
                db.rollback();
        } else {
            fail(q.lastError(), error);
        }
    }

    // DETACH solo quando non restano query aperte su "other".
    QSqlQuery(db).exec("DETACH DATABASE other");
    return result;
}

bool Database::openAppDatabase(const QString &dataDir, QString *error)
{
    const QString path = dataDir + "/vaultly.db";
    // La 1.0.3 impostava l'organizationName e Qt metteva i dati in <AppData>/Vaultly/Vaultly.
    const QString misplacedDir = dataDir + "/Vaultly";
    const QString misplaced = misplacedDir + "/vaultly.db";

    QDir().mkpath(dataDir);
    if (QFile::exists(misplaced) && !QFile::exists(path)) {
        if (!QFile::rename(misplaced, path)) {
            if (error)
                *error = QStringLiteral("Impossibile spostare %1").arg(misplaced);
            return false;
        }
        QDir().rmdir(misplacedDir);
    }

    if (!open(path, error))
        return false;

    if (QFile::exists(misplaced)) {
        int conflicts = 0;
        // Se l'import fallisce il file resta dov'è e si riprova al prossimo avvio.
        if (importUsersFrom(misplaced, &conflicts)) {
            if (conflicts == 0) {
                QFile::remove(misplaced);
            } else {
                // Utenti con lo stesso nome in entrambi: non si sovrascrive nulla, il file si tiene.
                const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
                QFile::rename(misplaced, QStringLiteral("%1/vaultly-recupero-%2.db").arg(dataDir, stamp));
            }
            QDir().rmdir(misplacedDir);
        }
    }
    return true;
}

void Database::close()
{
    const QString name = QString::fromLatin1(QSqlDatabase::defaultConnection);
    {
        QSqlDatabase db = QSqlDatabase::database(name, false);
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
}
