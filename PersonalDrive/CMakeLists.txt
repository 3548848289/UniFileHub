set(SCHEDULE_SOURCES
   DriveView.cpp
   DriveApiClient.cpp
   DriveItem.cpp
   DriveFile.cpp
   DriveFolder.cpp
   DriveManager.cpp
   DriveViewDelegate.cpp
)

set(SCHEDULE_HEADERS
    DriveView.h
    include/DriveApiClient.h
    include/DriveItem.h
    include/DriveFile.h
    include/DriveFolder.h
    include/DriveManager.h
    include/DriveViewDelegate.h
)

qt_add_library(mytxt_drive STATIC
    ${SCHEDULE_SOURCES}
    ${SCHEDULE_HEADERS}
    DriveView.ui
)

target_link_libraries(mytxt_drive PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(mytxt_drive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

