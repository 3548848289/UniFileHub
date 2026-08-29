cmake_minimum_required(VERSION 3.5)
project(UniFileHub VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UNIFILEHUB_QT_MAJOR "AUTO" CACHE STRING "Qt major version to build UniFileHub with: AUTO, 5, or 6")
set_property(CACHE UNIFILEHUB_QT_MAJOR PROPERTY STRINGS AUTO 5 6)

if(UNIFILEHUB_QT_MAJOR STREQUAL "6")
    find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Sql Network Svg SvgWidgets Multimedia MultimediaWidgets Pdf PdfWidgets LinguistTools)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Svg SvgWidgets Multimedia MultimediaWidgets Pdf PdfWidgets LinguistTools)
elseif(UNIFILEHUB_QT_MAJOR STREQUAL "5")
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg Multimedia MultimediaWidgets LinguistTools)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg Multimedia MultimediaWidgets LinguistTools)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg Multimedia MultimediaWidgets LinguistTools)
    if(QT_VERSION_MAJOR EQUAL 6)
        find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Svg SvgWidgets Multimedia MultimediaWidgets Pdf PdfWidgets LinguistTools)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg Multimedia MultimediaWidgets LinguistTools)
    endif()
endif()
set(UNIFILEHUB_ENABLE_TERMINAL_DEFAULT OFF)
if(QT_VERSION_MAJOR EQUAL 6)
    if(NOT CMAKE_VERSION VERSION_LESS 3.11)
        set(UNIFILEHUB_ENABLE_TERMINAL_DEFAULT ON)
    endif()
endif()
option(UNIFILEHUB_ENABLE_TERMINAL "Build the embedded terminal" ${UNIFILEHUB_ENABLE_TERMINAL_DEFAULT})

if(UNIFILEHUB_ENABLE_TERMINAL)
    if(NOT QT_VERSION_MAJOR EQUAL 6)
        message(WARNING "Embedded terminal requires Qt6; disabling it for this build.")
        set(UNIFILEHUB_ENABLE_TERMINAL OFF CACHE BOOL "Build the embedded terminal" FORCE)
    elseif(CMAKE_VERSION VERSION_LESS 3.11)
        message(WARNING "Embedded terminal requires CMake 3.11 or newer; disabling it for this build.")
        set(UNIFILEHUB_ENABLE_TERMINAL OFF CACHE BOOL "Build the embedded terminal" FORCE)
    endif()
endif()

if(UNIFILEHUB_ENABLE_TERMINAL)
    add_definitions(-DUNIFILEHUB_ENABLE_TERMINAL=1)
else()
    add_definitions(-DUNIFILEHUB_ENABLE_TERMINAL=0)
endif()


# 启用自动处理 Qt 的 uic, moc, 和 rcc
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(QT_VERSION_MAJOR EQUAL 5)
    macro(qt_add_library target)
        add_library(${target} ${ARGN})
    endmacro()

    macro(qt_add_executable target)
        add_executable(${target} ${ARGN})
    endmacro()
endif()


# 添加第三方库（需要在 main 之前构建）
add_subdirectory(Resources/ThirdParty/simple-mail/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/simple-mail)

if(QT_VERSION_MAJOR EQUAL 6)
    add_subdirectory(Resources/ThirdParty/QXlsx/
        ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/QXlsx)
endif()

add_subdirectory(Resources/ThirdParty/LmsgNotify/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/LmsgNotify)

add_subdirectory(Resources/ThirdParty/QFileSystemBreadcrumbBar/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/QFileSystemBreadcrumbBar)

add_subdirectory(Resources/ThirdParty/libqpsd/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/libqpsd)

if(UNIFILEHUB_ENABLE_TERMINAL)
    add_subdirectory(Resources/ThirdParty/KodoTerm/
        ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/KodoTerm)
endif()



# 添加子目录
add_subdirectory(manager)
add_subdirectory(FileBackup)
add_subdirectory(FileSystem)
add_subdirectory(OnlineDoc)
add_subdirectory(TagSchedule)
add_subdirectory(Setting)
add_subdirectory(UserInfo)
add_subdirectory(PersonalDrive)
add_subdirectory(EmailService)
add_subdirectory(ClipBoard)
add_subdirectory(MoreFunction)
add_subdirectory(DisposeAbstract/DisposeTXT)
add_subdirectory(DisposeAbstract/DisposeCSV)
add_subdirectory(DisposeAbstract/DisposeIMG)
add_subdirectory(DisposeAbstract/DisposeVideo)
add_subdirectory(DisposeAbstract/DisposePDF)
add_subdirectory(DisposeAbstract/DisposeDB)
add_subdirectory(DisposeAbstract/DisposePSD)
if(QT_VERSION_MAJOR EQUAL 6)
    add_subdirectory(DisposeAbstract/DisposeXLSX)
endif()
add_subdirectory(main)


if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UniFileHubIcon.rc")
endif()

qt_add_executable(UniFileHub WIN32 Resources/res.qrc ${APP_ICON_RESOURCE_WINDOWS})


target_link_libraries(UniFileHub PRIVATE
    mytxt_main
    mytxt_filehis
    mytxt_filetag
    mytxt_onlinedoc
    mytxt_schedule
    mytxt_setting
    mytxt_userinfo
    mytxt_email
    mytxt_drive
    mytxt_txt
    mytxt_csv
    mytxt_img
    mytxt_video
    mytxt_pdf
    mytxt_db
    mytxt_psd
    mytxt_manager
    mytxt_morefunction
    mytxt_clipboard
    SimpleMail::Core
    LmsgNotify::LmsgNotify
    libqpsd
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(UniFileHub PRIVATE
        mytxt_xlsx
        QXlsx::QXlsx
    )
endif()

if(UNIFILEHUB_ENABLE_TERMINAL)
    target_link_libraries(UniFileHub PRIVATE KodoTerm::KodoTerm)
endif()

set(APP_TRANSLATION_TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/translations/UniFileHub_zh_CN.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/translations/UniFileHub_En.ts"
)

set(APP_TRANSLATION_SOURCE_TARGETS
    UniFileHub
    mytxt_main
    mytxt_filehis
    mytxt_filetag
    mytxt_onlinedoc
    mytxt_schedule
    mytxt_setting
    mytxt_userinfo
    mytxt_email
    mytxt_drive
    mytxt_txt
    mytxt_csv
    mytxt_img
    mytxt_video
    mytxt_pdf
    mytxt_db
    mytxt_psd
    mytxt_manager
    mytxt_morefunction
    mytxt_clipboard
)

if(QT_VERSION_MAJOR EQUAL 6)
    list(APPEND APP_TRANSLATION_SOURCE_TARGETS mytxt_xlsx)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
qt_add_lupdate(
    SOURCE_TARGETS ${APP_TRANSLATION_SOURCE_TARGETS}
    TS_FILES ${APP_TRANSLATION_TS_FILES}
    LUPDATE_TARGET update_translations
)

qt_add_lrelease(
    TS_FILES ${APP_TRANSLATION_TS_FILES}
    LRELEASE_TARGET release_translations
    QM_FILES_OUTPUT_VARIABLE APP_TRANSLATION_QM_FILES
)

add_dependencies(release_translations update_translations)

# 安装规则
add_dependencies(UniFileHub release_translations)

add_custom_command(TARGET UniFileHub POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:UniFileHub>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${APP_TRANSLATION_QM_FILES}
        "$<TARGET_FILE_DIR:UniFileHub>/translations"
    COMMENT "Copying translation files to runtime directory"
)

endif()

include(GNUInstallDirs)
install(TARGETS UniFileHub
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(QT_VERSION_MAJOR EQUAL 6)
    install(FILES ${APP_TRANSLATION_QM_FILES}
        DESTINATION ${CMAKE_INSTALL_BINDIR}/translations
    )
endif()

# 如果使用 Qt6，则需要调用 qt_finalize_executable
