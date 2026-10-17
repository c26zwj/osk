cmake_minimum_required(VERSION 3.20)
project(orbit-osk VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets DBus GuiPrivate)
find_package(LayerShellQt REQUIRED)

qt_add_executable(orbit-osk src/main.cpp)
target_link_libraries(orbit-osk PRIVATE Qt6::Widgets Qt6::DBus Qt6::GuiPrivate
                        LayerShellQt::Interface wayland-client)

install(TARGETS orbit-osk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/orbit-osk.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
