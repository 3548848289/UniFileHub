set(SCHEDULE_SOURCES
   TabHandleCSV.cpp
)

set(SCHEDULE_HEADERS
    TabHandleCSV.h
)

qt_add_library(mytxt_csv STATIC
    ${SCHEDULE_SOURCES}
    ${SCHEDULE_HEADERS}
)

target_link_libraries(mytxt_csv PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(mytxt_csv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(mytxt_csv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
