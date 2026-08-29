cmake_minimum_required(VERSION 3.10)
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()



set(MAIN_SOURCES
    main.cpp
    mainwindow.cpp
    RecentFilesManager.cpp
    WidgetFunctional.cpp
    FindDialog.cpp
    TabManager.cpp
    TabFactory.cpp


    include/mainwindow.h
    include/TabAbstract.h
    include/RecentFilesManager.h
    include/WidgetFunctional.h
    include/FindDialog.h
    include/TabFactory.h
    include/TabManager.h

    ui/WidgetFunctional.ui
    ui/mainwindow.ui
    ui/FindDialog.ui

)

# 查找 Qt 库
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network Pdf PdfWidgets)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Pdf PdfWidgets)
else()
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets Network)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
endif()

# 创建静态库
qt_add_library(mytxt_main STATIC
    ${MAIN_SOURCES}
    ../Resources/res.qrc
)

# 添加资源文件

# 包含头文件目录
target_include_directories(mytxt_main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include

)

# 链接 Qt 库和 KodoTerm
target_link_libraries(mytxt_main PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(mytxt_main PRIVATE
        Qt6::Pdf
        Qt6::PdfWidgets
    )
endif()

if(UNIFILEHUB_ENABLE_TERMINAL)
    target_link_libraries(mytxt_main PRIVATE KodoTerm::KodoTerm)
endif()
