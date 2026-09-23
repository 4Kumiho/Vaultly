#include "platform/Updater.h"

#include "Version.h"

#include <QCoreApplication>
#include <QLibrary>

#include <string>

namespace {

constexpr int kCheckIntervalSeconds = 24 * 60 * 60;

// Firme delle funzioni di winsparkle.h usate qui.
using VoidFn = void(__cdecl *)();
using SetStringFn = void(__cdecl *)(const char *);
using SetKeyFn = int(__cdecl *)(const char *);
using SetDetailsFn = void(__cdecl *)(const wchar_t *, const wchar_t *, const wchar_t *);
using SetIntFn = void(__cdecl *)(int);
using CanShutdownCallback = int(__cdecl *)();
using ShutdownCallback = void(__cdecl *)();
using SetCanShutdownFn = void(__cdecl *)(CanShutdownCallback);
using SetShutdownFn = void(__cdecl *)(ShutdownCallback);

QLibrary *library = nullptr;
bool started = false;

template <typename Fn>
Fn resolve(const char *name)
{
    return reinterpret_cast<Fn>(library->resolve(name));
}

// Chiamate da un thread di WinSparkle prima di lanciare l'installer.
int __cdecl canShutdown()
{
    return 1;
}

void __cdecl requestShutdown()
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), &QCoreApplication::quit, Qt::QueuedConnection);
}

} // namespace

void Updater::start()
{
    const char *feedUrl = BANKEVIOUR_UPDATE_FEED_URL;
    if (started || feedUrl[0] == '\0')
        return;

    library = new QLibrary(QCoreApplication::applicationDirPath() + "/WinSparkle.dll");
    if (!library->load()) {
        delete library;
        library = nullptr;
        return;
    }

    const auto setLang = resolve<SetStringFn>("win_sparkle_set_lang");
    const auto setAppcastUrl = resolve<SetStringFn>("win_sparkle_set_appcast_url");
    const auto setPublicKey = resolve<SetKeyFn>("win_sparkle_set_eddsa_public_key");
    const auto setAppDetails = resolve<SetDetailsFn>("win_sparkle_set_app_details");
    const auto setAutomaticCheck = resolve<SetIntFn>("win_sparkle_set_automatic_check_for_updates");
    const auto setInterval = resolve<SetIntFn>("win_sparkle_set_update_check_interval");
    const auto setCanShutdown = resolve<SetCanShutdownFn>("win_sparkle_set_can_shutdown_callback");
    const auto setShutdownRequest = resolve<SetShutdownFn>("win_sparkle_set_shutdown_request_callback");
    const auto init = resolve<VoidFn>("win_sparkle_init");
    if (!setLang || !setAppcastUrl || !setPublicKey || !setAppDetails || !setAutomaticCheck || !setInterval
        || !setCanShutdown || !setShutdownRequest || !init)
        return;

    // Senza una chiave pubblica valida non si parte: niente aggiornamenti non firmati.
    if (!setPublicKey(BANKEVIOUR_UPDATE_PUBLIC_KEY))
        return;

    const std::wstring version = QStringLiteral(BANKEVIOUR_VERSION).toStdWString();
    setLang("it");
    setAppcastUrl(feedUrl);
    setAppDetails(L"Bankeviour", L"Bankeviour", version.c_str());
    setCanShutdown(&canShutdown);
    setShutdownRequest(&requestShutdown);
    setAutomaticCheck(1);
    setInterval(kCheckIntervalSeconds);
    init();
    started = true;
}

void Updater::stop()
{
    if (!started)
        return;
    if (const auto cleanup = resolve<VoidFn>("win_sparkle_cleanup"))
        cleanup();
    started = false;
}

bool Updater::isAvailable()
{
    return started;
}

void Updater::checkNow()
{
    if (!started)
        return;
    if (const auto check = resolve<VoidFn>("win_sparkle_check_update_with_ui"))
        check();
}
