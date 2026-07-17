# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Dtk6 REQUIRED COMPONENTS Core Gui Tools Widget)

add_executable(appearance-opacity-example
    main.cpp
    framebufferprobe.cpp
    framebufferprobe.h
    opacitymonitor.cpp
    opacitymonitor.h
    qml.qrc
)

dtk_add_dbus_interface(DBusInterface
    ${PROJECT_SOURCE_DIR}/applets/dde-appearance/org.deepin.dde.Appearance1.xml
    Appearance1
)
target_sources(appearance-opacity-example PRIVATE ${DBusInterface})

target_link_libraries(appearance-opacity-example PRIVATE
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
    Qt::DBus
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Widgets
)

configure_file(
    collect-opacity-logs.sh
    collect-opacity-logs.sh
    COPYONLY
)
