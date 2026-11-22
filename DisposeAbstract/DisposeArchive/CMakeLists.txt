set(ARCHIVE_SOURCES
    ArchiveReader.cpp
    TabHandleArchive.cpp
    ControlWidArchive.cpp
)

set(ARCHIVE_HEADERS
    ArchiveReader.h
    TabHandleArchive.h
    ControlWidArchive.h
)

qt_add_library(mytxt_archive STATIC
    ${ARCHIVE_SOURCES}
    ${ARCHIVE_HEADERS}
)

# 后台解压任务需要 QtConcurrent
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent)

target_link_libraries(mytxt_archive PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    archive_static
)

# 供 main/TabFactory.h 相对路径 include 使用
target_include_directories(mytxt_archive INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# archive.h 在 Windows 下默认按 dllimport 声明 API，静态链接需定义 LIBARCHIVE_STATIC
target_compile_definitions(mytxt_archive PRIVATE LIBARCHIVE_STATIC)

if(MSVC)
    # 本模块源码为 UTF-8 无 BOM，中文字面量需要 /utf-8
    target_compile_options(mytxt_archive PRIVATE /utf-8)
endif()
