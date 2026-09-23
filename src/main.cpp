#include "Version.h"
#include "db/Database.h"
#include "platform/Updater.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Bankeviour");
    QApplication::setApplicationVersion(BANKEVIOUR_VERSION);
    // Testi dell'app in italiano: date e importi seguono la stessa lingua.
    QLocale::setDefault(QLocale(QLocale::Italian, QLocale::Italy));
    Theme::apply(app);

    // Ogni utente di Windows ha il suo DB: %APPDATA%\Bankeviour\bankeviour.db
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QString error;
    if (!Database::open(dataDir + "/bankeviour.db", &error)) {
        QMessageBox::critical(nullptr, "Bankeviour",
                              QObject::tr("Impossibile aprire il database:\n%1").arg(error));
        return 1;
    }

    MainWindow window;
    window.show();
    Updater::start();

    const int result = app.exec();
    Updater::stop();
    return result;
}
