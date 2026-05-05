cmake_minimum_required(VERSION 3.5)
project(UniFileHub VERSION 0.1 LANGUAGES CXX)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Sql Network Svg SvgWidgets Multimedia MultimediaWidgets Pdf PdfWidgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Svg SvgWidgets Multimedia MultimediaWidgets Pdf PdfWidgets LinguistTools)

# 启用自动处理 Qt 的 uic, moc, 和 rcc
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


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
add_subdirectory(DisposeAbstract/DisposeXLSX)
add_subdirectory(DisposeAbstract/DisposeVideo)
add_subdirectory(DisposeAbstract/DisposePDF)
add_subdirectory(DisposeAbstract/DisposeDB)
add_subdirectory(DisposeAbstract/DisposePSD)
add_subdirectory(main)


add_subdirectory(Resources/ThirdParty/simple-mail/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/simple-mail)

add_subdirectory(Resources/ThirdParty/QXlsx/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/QXlsx)

add_subdirectory(Resources/ThirdParty/LmsgNotify/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/LmsgNotify)

add_subdirectory(Resources/ThirdParty/QFileSystemBreadcrumbBar/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/QFileSystemBreadcrumbBar)

add_subdirectory(Resources/ThirdParty/libqpsd/
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/ThirdParty/libqpsd)


qt_add_resources(RESOURCES
    Resources/res.qrc
)

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UniFileHubIcon.rc")
endif()

qt_add_executable(UniFileHub WIN32 ${RESOURCES} ${APP_ICON_RESOURCE_WINDOWS})


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
    mytxt_xlsx
    mytxt_video
    mytxt_pdf
    mytxt_db
    mytxt_psd
    mytxt_manager
    mytxt_morefunction
    mytxt_clipboard
    SimpleMail::Core
    QXlsx::QXlsx
    LmsgNotify::LmsgNotify
    libqpsd
    Qt${QT_VERSION_MAJOR}::Widgets
)

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
    mytxt_xlsx
    mytxt_video
    mytxt_pdf
    mytxt_db
    mytxt_psd
    mytxt_manager
    mytxt_morefunction
    mytxt_clipboard
)

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

include(GNUInstallDirs)
install(TARGETS UniFileHub
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${APP_TRANSLATION_QM_FILES}
    DESTINATION ${CMAKE_INSTALL_BINDIR}/translations
)

# 如果使用 Qt6，则需要调用 qt_finalize_executable
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(UniFileHub)
endif()
