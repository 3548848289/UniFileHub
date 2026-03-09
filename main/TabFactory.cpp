#include "TabFactory.h"
#include <QFileInfo>
#include <QMessageBox>
#include "../DisposeAbstract/DisposePSD/include/TabHandlePSD.h"

const QMap<QString, std::function<TabAbstract*(const QString&)>> TabFactory::factories = {
    { "txt",   [](const QString& f){ return new TextTab(f); } },
    { "cpp",   [](const QString& f){ return new TextTab(f); } },
    { "c",     [](const QString& f){ return new TextTab(f); } },
    { "h",     [](const QString& f){ return new TextTab(f); } },
    { "hpp",   [](const QString& f){ return new TextTab(f); } },
    { "cc",    [](const QString& f){ return new TextTab(f); } },
    { "java",  [](const QString& f){ return new TextTab(f); } },
    { "py",    [](const QString& f){ return new TextTab(f); } },
    { "js",    [](const QString& f){ return new TextTab(f); } },
    { "ts",    [](const QString& f){ return new TextTab(f); } },
    { "html",  [](const QString& f){ return new TextTab(f); } },
    { "htm",   [](const QString& f){ return new TextTab(f); } },
    { "css",   [](const QString& f){ return new TextTab(f); } },
    { "xml",   [](const QString& f){ return new TextTab(f); } },
    { "qrc",   [](const QString& f){ return new TextTab(f); } },
    { "ini",   [](const QString& f){ return new TextTab(f); } },
    { "json",  [](const QString& f){ return new TextTab(f); } },
    { "yaml",  [](const QString& f){ return new TextTab(f); } },
    { "yml",   [](const QString& f){ return new TextTab(f); } },
    { "md",    [](const QString& f){ return new TextTab(f); } },
    { "log",   [](const QString& f){ return new TextTab(f); } },
    { "bat",   [](const QString& f){ return new TextTab(f); } },
    { "sh",    [](const QString& f){ return new TextTab(f); } },

    // --- 表格/数据类 ---
    { "csv",   [](const QString& f){ return new TabHandleCSV(f); } },
    { "tsv",   [](const QString& f){ return new TabHandleCSV(f); } },
    { "xlsx",  [](const QString& f){ return new TabHandleXLSX(f); } },
    { "xls",   [](const QString& f){ return new TabHandleXLSX(f); } },
    { "ods",   [](const QString& f){ return new TabHandleXLSX(f); } },

    // --- 图片类 ---
    { "png",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "jpg",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "jpeg",  [](const QString& f){ return new TabHandleIMG(f); } },
    { "bmp",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "svg",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "gif",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "webp",  [](const QString& f){ return new TabHandleIMG(f); } },
    { "tiff",  [](const QString& f){ return new TabHandleIMG(f); } },
    { "ico",   [](const QString& f){ return new TabHandleIMG(f); } },
    { "psd",  [](const QString& f){ return new TabHandlePSD(f); } },
    { "ai",   [](const QString& f){ return new TabHandleIMG(f); } },
    
    // --- XMind 思维导图类 ---
    { "xmind", [](const QString& f){ return new TabHandleIMG(f); } },

    // --- 视频类 ---
    { "mp4",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "avi",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "mov",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "wmv",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "mkv",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "flv",   [](const QString& f){ return new TabHandleVideo(f); } },
    { "webm",  [](const QString& f){ return new TabHandleVideo(f); } },
    { "mp3",   [](const QString& f){ return new TabHandleVideo(f); } },

    // --- PDF类 ---
    { "pdf",  [](const QString& f){ return new TabHandlePDF(f); } },
    { "ai",  [](const QString& f){ return new TabHandlePDF(f); } },
    
    // --- 数据库类 ---
    { "db",   [](const QString& f){ return new TabHandleDB(f); } },
    { "sqlite", [](const QString& f){ return new TabHandleDB(f); } },
    { "sqlite3", [](const QString& f){ return new TabHandleDB(f); } },

    { "psd", [](const QString& f){ return new TabHandlePSD(f); } },

};

TabAbstract* TabFactory::create(const QString& fileName) {
    QString ext = QFileInfo(fileName).suffix().toLower();
    auto it = factories.find(ext);
    if (it != factories.end()) {
        return it.value()(fileName);
    }
    
    // 处理没有扩展名的文件
    if (ext.isEmpty()) {
        // 尝试以文本方式打开没有扩展名的文件
        return new TextTab(fileName);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, QObject::tr("不受支持的文件类型"),
        QObject::tr("该文件类型不受支持。是否以文本方式打开？"),
        QMessageBox::Yes | QMessageBox::No);

    return (reply == QMessageBox::Yes) ? new TextTab(fileName) : nullptr;
}
