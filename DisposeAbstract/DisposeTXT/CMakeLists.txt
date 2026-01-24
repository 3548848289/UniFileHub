set(SCHEDULE_SOURCES
   ControlWidTXT.cpp
   TabHandleTXT.cpp
   PlainTextEdit.cpp
   LineNumberWidget.cpp
   SyntaxHighlighter.cpp
)

set(SCHEDULE_HEADERS
    ControlWidTXT.h
    TabHandleTXT.h
    PlainTextEdit.h
    LineNumberWidget.h
    SyntaxHighlighter.h
)


qt_add_library(mytxt_txt STATIC
    ${SCHEDULE_SOURCES}
    ${SCHEDULE_HEADERS}
    ControlWidTXT.ui
)

target_link_libraries(mytxt_txt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Qt 6 需要 Qt5Compat 模块来使用 QTextCodec
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS Core5Compat)
    if(Qt6Core5Compat_FOUND)
        target_link_libraries(mytxt_txt PRIVATE Qt6::Core5Compat)
    endif()
endif()

target_include_directories(mytxt_txt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
