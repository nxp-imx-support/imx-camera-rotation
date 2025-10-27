# Copyright (C) 2022 The Qt Company Ltd.
# Copyright 2024-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.14)

project(camera_rotation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick)

pkg_check_modules(UDEV REQUIRED libudev)

set(PROJECT_SOURCES
        cpp/videodevice.cpp cpp/videodevice.hpp
        cpp/mqueuethread.cpp cpp/mqueuethread.hpp
        cpp/mediastream.cpp cpp/mediastream.hpp
        cpp/main.cpp
        qrc/icons.qrc
        qrc/qml.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(camera_rotation
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
endif()

target_compile_definitions(camera_rotation
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(camera_rotation
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Quick
)

target_include_directories(camera_rotation PUBLIC cpp/)
target_include_directories(camera_rotation PRIVATE ${UDEV_INCLUDE_DIRS})

target_link_libraries(camera_rotation
    PRIVATE
    ${UDEV_LIBRARIES}
)

qt_add_qml_module(camera_rotation
    URI MediaStream
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

set_target_properties(camera_rotation PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(camera_rotation)
    qt_finalize_executable(camera_rotation)
endif()
