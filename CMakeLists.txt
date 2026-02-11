cmake_minimum_required(VERSION 3.25)
project(osk VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --- Find ECM first ---
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# --- Qt6 ---
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Widgets DBus)

# --- KDE ---
find_package(LayerShellQt REQUIRED)
find_package(KF6StatusNotifierItem REQUIRED)
find_package(KF6GlobalAccel REQUIRED)

# --- Executable ---
add_executable(osk
    src/main.cpp
    src/virtualkeyboard.cpp
    src/keyboardcontroller.cpp
    resources.qrc
)

# --- Link ---
target_link_libraries(osk PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::DBus
    LayerShellQt::Interface
    KF6::StatusNotifierItem
    KF6::GlobalAccel
)

target_include_directories(osk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
