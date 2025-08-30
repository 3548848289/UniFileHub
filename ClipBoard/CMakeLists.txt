set(SETTING_SOURCES

    ClipboardItem/CliText.cpp
    ClipboardItem/CliImage.cpp
    ClipboardItem/CliFile.cpp
    ClipboardItemFactory.cpp

    FiletypeDetector.cpp
    ImagePreviewDialog.cpp

    ClipboardView.cpp
)

set(SETTING_HEADERS
    include/ClipboardItem/ClipboardItem.h
    include/ClipboardItem/CliText.h
    include/ClipboardItem/CliImage.h
    include/ClipboardItem/CliFile.h
    include/ClipboardItemFactory.h

    include/FiletypeDetector.h
    include/ImagePreviewDialog.h

    include/ClipboardView.h
)

set(SETTING_UIS
    ui/ClipboardView.ui
)

qt_add_library(mytxt_clipboard STATIC
    ${SETTING_SOURCES}
    ${SETTING_HEADERS}
    ${SETTING_UIS}

)

target_link_libraries(mytxt_clipboard PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(mytxt_clipboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
