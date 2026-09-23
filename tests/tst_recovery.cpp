#include "core/AccountService.h"
#include "core/AuthService.h"
#include "core/TransactionService.h"
#include "core/VaultService.h"
#include "db/AccountRepository.h"
#include "db/CategoryRepository.h"
#include "db/Database.h"
#include "db/TransactionRepository.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

// Recupero del DB finito in <dati>/Vaultly/vaultly.db con la 1.0.3.
namespace {

const Currency kEur{"EUR", "€", 2};

// Crea un utente con un conto, un'uscita e una voce nell'area password nel DB aperto.
void addUser(const QString &name, qint64 initialBalance, qint64 expense)
{
    const Session s = AuthService::registerUser(name, "segreto1", "segreto1").session();
    const auto account = AccountService::create(s.user.id, "Conto", kEur, initialBalance).account;
    Transaction t;
    t.accountId = account->id;
    t.type = TransactionType::Expense;
    for (const Category &c : CategoryRepository::all())
        if (c.name == "Spesa" && c.type == TransactionType::Expense) t.categoryId = c.id;
    t.amount = expense;
    t.occurredAt = QDateTime(QDate(2026, 9, 1), QTime(10, 0));
    t.tags = {"etichetta di " + name};
    TransactionService::create(s.user.id, t);
    VaultEntry e;
    e.title = "Gmail di " + name;
    e.password = "pw-" + name;
    VaultService::create(s, e);
}

void createDb(const QString &path, const QStringList &users)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QVERIFY(Database::open(path));
    for (const QString &u : users)
        addUser(u, 10000, 2500);
    Database::close();
}

int userCount()
{
    QSqlQuery q("SELECT COUNT(*) FROM users");
    return q.next() ? q.value(0).toInt() : -1;
}

// Login, saldo, etichette e area password di un utente nel DB aperto.
void checkUser(const QString &name, bool withTags = true)
{
    const auto login = AuthService::login(name, "segreto1");
    QVERIFY2(login.user, qPrintable(name));
    const auto accounts = AccountRepository::listForUser(login.user->id);
    QCOMPARE(accounts.size(), 1);
    QCOMPARE(AccountRepository::currentBalance(accounts[0].id, login.user->id), qint64(7500));
    const auto txs = TransactionRepository::listForAccount(accounts[0].id, login.user->id);
    QCOMPARE(txs.size(), 1);
    QCOMPARE(txs[0].tags, withTags ? QStringList({"etichetta di " + name}) : QStringList());
    const auto vault = VaultService::list(login.session());
    QCOMPARE(vault.size(), 1);
    QCOMPARE(vault[0].password, "pw-" + name);
}

} // namespace

class TestRecovery : public QObject
{
    Q_OBJECT

private slots:
    void cleanup() { Database::close(); }

    void freshInstallIsEmpty()
    {
        QTemporaryDir dir;
        QVERIFY(Database::openAppDatabase(dir.path()));
        QCOMPARE(userCount(), 0);
        QVERIFY(QFile::exists(dir.filePath("vaultly.db")));
    }

    void onlyMisplacedDatabaseIsMoved()
    {
        QTemporaryDir dir;
        createDb(dir.filePath("Vaultly/vaultly.db"), {"anna"});

        QVERIFY(Database::openAppDatabase(dir.path()));
        QCOMPARE(userCount(), 1);
        checkUser("anna");
        QVERIFY(!QFile::exists(dir.filePath("Vaultly/vaultly.db")));
        QVERIFY(!QDir(dir.filePath("Vaultly")).exists());
    }

    void bothDatabasesAreMerged()
    {
        QTemporaryDir dir;
        createDb(dir.filePath("vaultly.db"), {"mario"});
        createDb(dir.filePath("Vaultly/vaultly.db"), {"anna", "bruno"});

        QVERIFY(Database::openAppDatabase(dir.path()));
        QCOMPARE(userCount(), 3);
        for (const QString &u : {"mario", "anna", "bruno"})
            checkUser(u);
        QVERIFY(!QDir(dir.filePath("Vaultly")).exists());
        QVERIFY(QDir(dir.path()).entryList({"vaultly-recupero-*.db"}).isEmpty());
    }

    void conflictsAreKeptAside()
    {
        QTemporaryDir dir;
        createDb(dir.filePath("vaultly.db"), {"mario"});
        createDb(dir.filePath("Vaultly/vaultly.db"), {"MARIO", "anna"});

        QVERIFY(Database::openAppDatabase(dir.path()));
        QCOMPARE(userCount(), 2); // mario (originale) + anna
        checkUser("mario");
        checkUser("anna");
        // Il "MARIO" della 1.0.3 non sovrascrive nulla: il file resta per un eventuale recupero.
        QCOMPARE(QDir(dir.path()).entryList({"vaultly-recupero-*.db"}).size(), 1);
        QVERIFY(!QFile::exists(dir.filePath("Vaultly/vaultly.db")));
    }

    void importsVersion2Database()
    {
        // Il DB della 1.0.3 è schema v2: senza tabelle delle etichette.
        QTemporaryDir dir;
        const QString misplaced = dir.filePath("Vaultly/vaultly.db");
        createDb(dir.filePath("vaultly.db"), {"mario"});
        createDb(misplaced, {"anna"});
        QVERIFY(Database::open(misplaced));
        {
            QSqlQuery q;
            QVERIFY(q.exec("DROP TABLE transaction_tags"));
            QVERIFY(q.exec("DROP TABLE tags"));
            QVERIFY(q.exec("PRAGMA user_version = 2"));
        }
        Database::close();

        QVERIFY(Database::openAppDatabase(dir.path()));
        QCOMPARE(userCount(), 2);
        checkUser("mario");
        checkUser("anna", false);
        QVERIFY(!QFile::exists(misplaced));
    }

    void reopeningDoesNotDuplicate()
    {
        QTemporaryDir dir;
        createDb(dir.filePath("vaultly.db"), {"mario"});
        createDb(dir.filePath("Vaultly/vaultly.db"), {"anna"});
        QVERIFY(Database::openAppDatabase(dir.path()));
        Database::close();
        QVERIFY(Database::openAppDatabase(dir.path()));
        QCOMPARE(userCount(), 2);
    }
};

QTEST_GUILESS_MAIN(TestRecovery)
#include "tst_recovery.moc"
