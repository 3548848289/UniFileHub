cmake_minimum_required(VERSION 3.5)

project(QFileSystemBreadcrumbBar LANGUAGES CXX)

file(GLOB QFileSystemBreadcrumbBar_SOURCES
    *.cpp
)

file(GLOB QFileSystemBreadcrumbBar_HEADERS
    *.h
)

add_library(QFileSystemBreadcrumbBar STATIC
    ${QFileSystemBreadcrumbBar_SOURCES}
    ${QFileSystemBreadcrumbBar_HEADERS}
)

target_include_directories(QFileSystemBreadcrumbBar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QFileSystemBreadcrumbBar PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

# 给库定义一个别名用于引用
add_library(QFileSystemBreadcrumbBar::QFileSystemBreadcrumbBar ALIAS QFileSystemBreadcrumbBar)
