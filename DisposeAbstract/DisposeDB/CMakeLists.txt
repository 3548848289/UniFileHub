set(DB_SOURCES
    TabHandleDB.cpp
    ControlWidDB.cpp
)

set(DB_HEADERS
    TabHandleDB.h
    ControlWidDB.h
)

set(DB_FORMS
    ControlWidDB.ui
)


qt_add_library(mytxt_db STATIC
    ${DB_SOURCES}
    ${DB_HEADERS}
    ${DB_FORMS}
)

# 链接Qt的SQL库，用于SQLite数据库操作
target_link_libraries(mytxt_db
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

# 设置头文件搜索路径
target_include_directories(mytxt_db PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
