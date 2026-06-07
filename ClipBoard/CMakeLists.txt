set(SETTING_SOURCES

    ClipboardItem/CliText.cpp
    ClipboardItem/CliImage.cpp
    ClipboardItem/CliFile.cpp
    ClipboardItemFactory.cpp
    ClipboardHistoryManager.cpp

    FileTypeDetector.cpp
    ImagePreviewDialog.cpp

    ClipboardView.cpp
    ClipboardMenuBuilder.cpp
    ClipboardMonitor.cpp
    ClipboardController.cpp
    ClipboardComponentFactory.cpp
    ClipboardItemDelegate.cpp
    ClipboardCloudClient.cpp
)

set(SETTING_HEADERS
    include/ClipboardItem/ClipboardItem.h
    include/ClipboardItem/CliText.h
    include/ClipboardItem/CliImage.h
    include/ClipboardItem/CliFile.h
    include/ClipboardItemFactory.h

    include/FileTypeDetector.h
    include/ImagePreviewDialog.h

    include/ClipboardView.h
    include/ClipboardHistoryManager.h
    include/ClipboardMenuBuilder.h
    include/ClipboardMonitor.h
    include/ClipboardController.h
    include/ClipboardComponentFactory.h
    include/ClipboardItemDelegate.h
    include/ClipboardCloudClient.h

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
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(mytxt_clipboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
