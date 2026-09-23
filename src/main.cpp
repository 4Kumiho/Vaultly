#include "Version.h"
#include "db/Database.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Vaultly");
    // NON impostare organizationName: Qt lo aggiunge alla cartella dei dati
    // (%APPDATA%\Vaultly\Vaultly) e l'app non troverebbe più il DB. È successo nella 1.0.3.
    QApplication::setApplicationVersion(VAULTLY_VERSION);
    QApplication::setWindowIcon(QIcon(":/assets/vaultly.png"));
    // Testi dell'app in italiano: date e importi seguono la stessa lingua.
    QLocale::setDefault(QLocale(QLocale::Italian, QLocale::Italy));
    Theme::apply(app);

    // Ogni utente di Windows ha il suo DB: %APPDATA%\Vaultly\vaultly.db (vuoto alla prima installazione,
    // cancellato dalla disinstallazione).
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QString error;
    if (!Database::openAppDatabase(dataDir, &error)) {
        QMessageBox::critical(nullptr, "Vaultly",
                              QObject::tr("Impossibile aprire il database:\n%1").arg(error));
        return 1;
    }

    MainWindow window;
    window.show();
    window.startUpdateChecks();
    return app.exec();
}
