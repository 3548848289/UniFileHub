set(SCHEDULE_SOURCES
   DriveView.cpp
   DriveApiClient.cpp
   DriveItem.cpp
   DriveFile.cpp
   DriveFolder.cpp
   DriveManager.cpp
   DriveViewDelegate.cpp
   HistoryDelegate.cpp
)

set(SCHEDULE_HEADERS
    include/DriveView.h
    include/DriveApiClient.h
    include/DriveItem.h
    include/DriveFile.h
    include/DriveFolder.h
    include/DriveManager.h
    include/DriveViewDelegate.h
    include/DriveRoles.h
    include/HistoryDelegate.h

)

qt_add_library(mytxt_drive STATIC
    ${SCHEDULE_SOURCES}
    ${SCHEDULE_HEADERS}
    DriveView.ui
)

target_link_libraries(mytxt_drive PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::Pdf
    Qt${QT_VERSION_MAJOR}::PdfWidgets
    mytxt_setting
)

target_include_directories(mytxt_drive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

