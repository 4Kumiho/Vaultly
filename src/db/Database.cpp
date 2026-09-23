#include "db/Database.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

constexpr int kSchemaVersion = 2;

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

bool createSchemaV2(QSqlDatabase &db, QString *error)
{
    QSqlQuery q(db);
    for (const char *sql : kSchemaV2) {
        if (!q.exec(QString::fromUtf8(sql)))
            return fail(q.lastError(), error);
    }
    return true;
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
    if ((version < 1 && !createSchemaV1(db, error)) || (version < 2 && !createSchemaV2(db, error))) {
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

void Database::close()
{
    const QString name = QString::fromLatin1(QSqlDatabase::defaultConnection);
    {
        QSqlDatabase db = QSqlDatabase::database(name, false);
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
}
