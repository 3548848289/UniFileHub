#include "ArchiveWriter.h"
#include <archive.h>
#include <archive_entry.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QObject>

namespace {

struct WalkItem
{
    QString absPath;
    QString relPath;   // zip 内相对路径，'/' 分隔（目录以 '/' 结尾）
    bool isDir = false;
    qint64 size = 0;
};

// 收集一个根路径（文件或目录）的全部条目；文件夹以自身为 zip 根
void walkRoot(const QString &rootAbs, const QString &rootName, QList<WalkItem> &out)
{
    const QFileInfo info(rootAbs);
    if (info.isFile()) {
        WalkItem it;
        it.absPath = rootAbs;
        it.relPath = rootName;
        it.size = info.size();
        out.append(it);
        return;
    }
    if (!info.isDir())
        return;
    {
        WalkItem d;
        d.absPath = rootAbs;
        d.relPath = rootName + QLatin1Char('/');
        d.isDir = true;
        out.append(d);
    }
    QDirIterator it(rootAbs, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString abs = it.next();
        const QFileInfo fi = it.fileInfo();
        QString rel = QDir(rootAbs).relativeFilePath(abs);
        rel = rootName + QLatin1Char('/') + rel;
        WalkItem wi;
        wi.absPath = abs;
        wi.relPath = fi.isDir() ? rel + QLatin1Char('/') : rel;
        wi.isDir = fi.isDir();
        wi.size = fi.size();
        out.append(wi);
    }
}

} // namespace

bool ArchiveWriter::zipPaths(const QStringList &paths, const QString &destZipPath,
                             const ProgressCallback &progress, QString *error)
{
    QList<WalkItem> items;
    for (const QString &p : paths)
        walkRoot(QDir::cleanPath(p), QFileInfo(p).fileName(), items);
    if (items.isEmpty()) {
        if (error)
            *error = QObject::tr("没有可压缩的内容。");
        return false;
    }

    // 进度按字节：总量 = 全部文件条目大小之和
    quint64 totalBytes = 0;
    for (const WalkItem &wi : items) {
        if (!wi.isDir)
            totalBytes += wi.size;
    }

    struct archive *a = archive_write_new();
    if (!a) {
        if (error)
            *error = QObject::tr("无法创建归档写入器。");
        return false;
    }

    bool ok = true;
    do {
        if (archive_write_set_format_zip(a) != ARCHIVE_OK) {
            if (error)
                *error = QString::fromUtf8(archive_error_string(a));
            ok = false;
            break;
        }
        // 条目名按 UTF-8 存储并让写入器置上 UTF-8 标志（zip 规范 bit 11）
        if (archive_write_set_options(a, "hdrcharset=UTF-8") != ARCHIVE_OK) {
            if (error)
                *error = QString::fromUtf8(archive_error_string(a));
            ok = false;
            break;
        }
#ifdef Q_OS_WIN
        if (archive_write_open_filename_w(a, destZipPath.toStdWString().c_str()) != ARCHIVE_OK) {
#else
        if (archive_write_open_filename(a, QFile::encodeName(destZipPath).constData()) != ARCHIVE_OK) {
#endif
            if (error)
                *error = QString::fromUtf8(archive_error_string(a));
            ok = false;
            break;
        }

        const int total = items.size();
        quint64 doneBytes = 0;
        for (int i = 0; i < total; ++i) {
            const WalkItem &wi = items.at(i);
            if (progress && !progress(doneBytes, totalBytes, wi.relPath)) {
                if (error)
                    *error = QObject::tr("已取消。");
                ok = false;
                break;
            }

            struct archive_entry *entry = archive_entry_new();
            archive_entry_set_pathname_utf8(entry, wi.relPath.toUtf8().constData());
            archive_entry_set_filetype(entry, wi.isDir ? AE_IFDIR : AE_IFREG);
            archive_entry_set_perm(entry, wi.isDir ? 0755 : 0644);
            archive_entry_set_mtime(entry, time(nullptr), 0);
            if (!wi.isDir)
                archive_entry_set_size(entry, wi.size);

            if (archive_write_header(a, entry) != ARCHIVE_OK) {
                if (error)
                    *error = QString::fromUtf8(archive_error_string(a));
                archive_entry_free(entry);
                ok = false;
                break;
            }
            archive_entry_free(entry);

            if (wi.isDir) {
                archive_write_finish_entry(a);
                continue;
            }
            QFile in(wi.absPath);
            if (!in.open(QIODevice::ReadOnly)) {
                if (error)
                    *error = QObject::tr("无法读取文件：%1").arg(wi.absPath);
                ok = false;
                break;
            }
            char buf[65536];
            bool writeOk = true;
            while (!in.atEnd()) {
                // 大文件也逐块上报进度/取消，不等整个文件写完
                if (progress && !progress(doneBytes, totalBytes, wi.relPath)) {
                    if (error)
                        *error = QObject::tr("已取消。");
                    writeOk = false;
                    break;
                }
                const qint64 n = in.read(buf, sizeof(buf));
                if (n < 0
                    || archive_write_data(a, buf, static_cast<size_t>(n))
                           != static_cast<la_ssize_t>(n)) {
                    if (error)
                        *error = QObject::tr("写入归档数据失败：%1").arg(wi.absPath);
                    writeOk = false;
                    break;
                }
                doneBytes += static_cast<quint64>(n);
            }
            in.close();
            if (!writeOk) {
                ok = false;
                break;
            }
            if (archive_write_finish_entry(a) != ARCHIVE_OK) {
                if (error)
                    *error = QString::fromUtf8(archive_error_string(a));
                ok = false;
                break;
            }
        }
    } while (false);

    if (archive_write_close(a) != ARCHIVE_OK && ok) {
        if (error)
            *error = QString::fromUtf8(archive_error_string(a));
        ok = false;
    }
    archive_write_free(a);

    if (!ok) {
        // 清理半成品 zip
        QFile::remove(destZipPath);
        if (error && error->isEmpty())
            *error = QObject::tr("压缩失败。");
    }
    return ok;
}
