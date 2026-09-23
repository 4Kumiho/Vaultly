cmake_minimum_required(VERSION 3.21)
# Versione dell'app: si alza qui a ogni release (vedi scripts/release.ps1).
project(Vaultly VERSION 1.0.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Charts Network Test)
qt_standard_project_setup()

# --- Aggiornamenti automatici (platform/UpdateService) ---
set(VAULTLY_GITHUB_REPO "" CACHE STRING
    "Repository GitHub (proprietario/nome) delle release. Vuoto = aggiornamenti disattivati.")
if(VAULTLY_GITHUB_REPO)
    set(VAULTLY_UPDATE_FEED_URL
        "https://github.com/${VAULTLY_GITHUB_REPO}/releases/latest/download/appcast.xml")
else()
    set(VAULTLY_UPDATE_FEED_URL "")
endif()
# Chiave pubblica: può stare nel repository. La privata sta in %USERPROFILE%\.vaultly.
set(VAULTLY_UPDATE_PUBLIC_KEY "YgjC4rRMQjdPz1CfZzWaX7JYjNnO7yXSm7nfsL0MNZ8=")

# Monocypher (BSD/CC0), solo per verificare le firme Ed25519 degli aggiornamenti.
add_library(monocypher STATIC
    external/monocypher/monocypher.c
    external/monocypher/monocypher-ed25519.c
)
target_include_directories(monocypher PUBLIC external/monocypher)

configure_file(src/Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h" @ONLY)

# Logica e accesso al DB: nessuna dipendenza dai widget, così i test la usano direttamente.
qt_add_library(vaultly_core STATIC
    src/core/Account.h
    src/core/AccountService.h
    src/core/AccountService.cpp
    src/core/Appcast.h
    src/core/Appcast.cpp
    src/core/AuthService.h
    src/core/AuthService.cpp
    src/core/BalanceHistory.h
    src/core/BalanceHistory.cpp
    src/core/Crypto.h
    src/core/Crypto.cpp
    src/core/Currency.h
    src/core/Money.h
    src/core/Money.cpp
    src/core/Transaction.h
    src/core/TransactionService.h
    src/core/TransactionService.cpp
    src/core/User.h
    src/core/VaultEntry.h
    src/core/VaultService.h
    src/core/VaultService.cpp
    src/db/AccountRepository.h
    src/db/AccountRepository.cpp
    src/db/CategoryRepository.h
    src/db/CategoryRepository.cpp
    src/db/CurrencyRepository.h
    src/db/CurrencyRepository.cpp
    src/db/Database.h
    src/db/Database.cpp
    src/db/TransactionRepository.h
    src/db/TransactionRepository.cpp
    src/db/UserRepository.h
    src/db/UserRepository.cpp
    src/db/VaultRepository.h
    src/db/VaultRepository.cpp
)
target_include_directories(vaultly_core PUBLIC src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(vaultly_core PUBLIC Qt6::Sql Qt6::Network bcrypt PRIVATE monocypher)

qt_add_executable(Vaultly WIN32
    src/app.rc
    src/main.cpp
    src/platform/UpdateService.h
    src/platform/UpdateService.cpp
    src/ui/AccountCard.h
    src/ui/AccountCard.cpp
    src/ui/AccountPanel.h
    src/ui/AccountPanel.cpp
    src/ui/AmountLabel.h
    src/ui/AmountLabel.cpp
    src/ui/Animations.h
    src/ui/Animations.cpp
    src/ui/BalanceChart.h
    src/ui/BalanceChart.cpp
    src/ui/Components.h
    src/ui/Components.cpp
    src/ui/DashboardPage.h
    src/ui/DashboardPage.cpp
    src/ui/HomePage.h
    src/ui/HomePage.cpp
    src/ui/LoginPage.h
    src/ui/LoginPage.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/RegisterPage.h
    src/ui/RegisterPage.cpp
    src/ui/SidePanel.h
    src/ui/SidePanel.cpp
    src/ui/SlideStack.h
    src/ui/SlideStack.cpp
    src/ui/Theme.h
    src/ui/Theme.cpp
    src/ui/Toast.h
    src/ui/Toast.cpp
    src/ui/TransactionList.h
    src/ui/TransactionList.cpp
    src/ui/TransactionPanel.h
    src/ui/TransactionPanel.cpp
    src/ui/UpdateOverlay.h
    src/ui/UpdateOverlay.cpp
    src/ui/VaultPage.h
    src/ui/VaultPage.cpp
    src/ui/VaultPanel.h
    src/ui/VaultPanel.cpp
)
target_link_libraries(Vaultly PRIVATE vaultly_core Qt6::Widgets Qt6::Charts)

# Icona (generata da tools/make-icon): nel .rc per l'exe, qui per finestra e schermate.
qt_add_resources(Vaultly "assets" PREFIX "/" FILES assets/vaultly.png)

enable_testing()

foreach(test tst_auth tst_money tst_accounts tst_transactions tst_balance tst_crypto tst_vault tst_update)
    qt_add_executable(${test} tests/${test}.cpp)
    target_link_libraries(${test} PRIVATE vaultly_core Qt6::Test)
    add_test(NAME ${test} COMMAND ${test})
endforeach()
