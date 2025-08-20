set(SCHEDULE_SOURCES
    TabHandleVideo.cpp
    ControlWidVideo.cpp
)

set(SCHEDULE_HEADERS
    TabHandleVideo.h
    ControlWidVideo.h
)


qt_add_library(mytxt_video STATIC
    ${SCHEDULE_SOURCES}
    ${SCHEDULE_HEADERS}
    ControlWidVideo.ui

)

target_link_libraries(mytxt_video PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
)

target_include_directories(mytxt_video PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
