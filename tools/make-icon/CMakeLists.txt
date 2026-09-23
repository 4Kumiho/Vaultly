# Genera assets/vaultly.png e assets/vaultly.ico. Non fa parte della build dell'app:
#   cmake -S tools/make-icon -B build-icon -G Ninja
#   cmake --build build-icon
#   build-icon\makeicon.exe assets      (con QT_QPA_PLATFORM=offscreen, lanciato da build-icon)
cmake_minimum_required(VERSION 3.21)
project(makeicon LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Gui)
qt_add_executable(makeicon main.cpp)
target_link_libraries(makeicon PRIVATE Qt6::Gui)
