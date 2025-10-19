set(SCHEDULE_SOURCES
   TabHandlePSD.cpp
   ControlWidPSD.cpp
)

set(SCHEDULE_HEADERS
    include/TabHandlePSD.h
    include/ControlWidPSD.h
)

qt_add_library(mytxt_psd STATIC
    ${SCHEDULE_SOURCES}
    ${SCHEDULE_HEADERS}
    ui/ControlWidPSD.ui
)

target_link_libraries(mytxt_psd PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::SvgWidgets
    libqpsd
)

target_include_directories(mytxt_psd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Resources/ThirdParty/libqpsd
)
