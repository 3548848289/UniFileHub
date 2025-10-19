cmake_minimum_required(VERSION 3.5)
project(libqpsd)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB LIBQPSD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB LIBQPSD_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(libqpsd STATIC ${LIBQPSD_SOURCES} ${LIBQPSD_HEADERS})

target_include_directories(libqpsd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libqpsd PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)
