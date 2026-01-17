set(SETTING_SOURCES
    Setting.cpp
    SettingManager.cpp
    AppHelp.cpp
    IconManager.cpp
    ThemeManager.cpp
)

set(SETTING_HEADERS
    include/Setting.h
    include/SettingManager.h
    include/AppHelp.h
    include/IconManager.h
    include/ThemeManager.h

)

set(SETTING_UIS
    ui/Setting.ui
    ui/AppHelp.ui
)

qt_add_library(mytxt_setting STATIC
    ${SETTING_SOURCES}
    ${SETTING_HEADERS}
    ${SETTING_UIS}
)

target_link_libraries(mytxt_setting PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

target_include_directories(mytxt_setting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
