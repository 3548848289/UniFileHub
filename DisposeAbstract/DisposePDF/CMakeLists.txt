set(PDF_SOURCES
    TabHandlePDF.cpp
    ControlWidPDF.cpp
)

set(PDF_HEADERS
    TabHandlePDF.h
    ControlWidPDF.h
)

qt_add_library(mytxt_pdf STATIC
    ${PDF_SOURCES}
    ${PDF_HEADERS}
    ControlWidPDF.ui
)

target_link_libraries(mytxt_pdf PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Pdf
    Qt${QT_VERSION_MAJOR}::PdfWidgets
)

target_include_directories(mytxt_pdf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
