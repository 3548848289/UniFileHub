#include "ArchiveReader.h"
#include <archive.h>
#include <archive_entry.h>
#include <cstring>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringDecoder>
#else
#include <QTextCodec>
#endif

namespace {

// 判断字节串是否为合法 UTF-8（用于无编码标志条目名的启发式解码）
bool looksLikeUtf8(const QByteArray &bytes)
{
    int i = 0;
    const int n = bytes.size();
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(bytes.at(i));
        if (c < 0x80) { ++i; continue; }
        int need = 0;
        if ((c & 0xE0) == 0xC0) need = 1;       // 2 字节
        else if ((c & 0xF0) == 0xE0) need = 2;  // 3 字节
        else if ((c & 0xF8) == 0xF0) need = 3;  // 4 字节
        else return false;
        if (i + need >= n) return false;
        for (int k = 1; k <= need; ++k) {
            const unsigned char cc = static_cast<unsigned char>(bytes.at(i + k));
            if ((cc & 0xC0) != 0x80) return false;
        }
        i += need + 1;
    }
    return true;
}

#ifdef Q_OS_WIN
// 用指定 Windows 代码页解码；失败（非法字节序）返回空串
QString decodeViaWinCodepage(const QByteArray &bytes, UINT codepage)
{
    const int wlen = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS,
                                         bytes.constData(), bytes.size(), nullptr, 0);
    if (wlen <= 0)
        return QString();
    QString result(wlen, QChar());
    MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS,
                        bytes.constData(), bytes.size(),
                        reinterpret_cast<wchar_t *>(result.data()), wlen);
    return result;
}
#endif

// GB18030（GBK 超集）解码，带回退。
// 注意：Qt6 官方 Windows 包无 ICU，QStringDecoder("GB18030") 不可用，
// 因此 Windows 上直接走系统 API（代码页 54936）。
QString decodeGb18030(const QByteArray &bytes)
{
#ifdef Q_OS_WIN
    const QString viaWin = decodeViaWinCodepage(bytes, 54936);
    if (!viaWin.isNull())
        return viaWin;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringDecoder decoder("GB18030");
    if (decoder.isValid())
        return decoder.decode(bytes); // EncodedData 隐式转换为 QString
    return QString::fromLocal8Bit(bytes);
#else
    if (QTextCodec *codec = QTextCodec::codecForName("GB18030"))
        return codec->toUnicode(bytes);
    return QString::fromLocal8Bit(bytes);
#endif
}

// 单个路径组件的非法字符清理
QString sanitizeComponent(const QString &component)
{
    QString out = component;
    static const QString illegal = QStringLiteral("<>:\"|?*");
    for (const QChar &ch : illegal)
        out.replace(ch, QLatin1Char('_'));
    while (out.endsWith(QLatin1Char('.')) || out.endsWith(QLatin1Char(' ')))
        out.chop(1);
    return out;
}

} // namespace

ArchiveReader::~ArchiveReader()
{
    close();
}

void ArchiveReader::close()
{
    m_open = false;
    m_entries.clear();
    m_formatName.clear();
    m_lastError.clear();
    m_sourcePath.clear();
    // m_data 保留（已解出的临时文件可能仍被预览标签页引用）
}

QString ArchiveReader::decodeName(const char *raw) const
{
    if (!raw)
        return QString();
    const QByteArray bytes(raw);
    if (looksLikeUtf8(bytes))
        return QString::fromUtf8(bytes);
    return decodeGb18030(bytes);
}

struct archive *ArchiveReader::newReader()
{
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    return a;
}

bool ArchiveReader::openSource(struct archive *a)
{
    int r;
#ifdef Q_OS_WIN
    if (!m_sourcePath.isEmpty())
        r = archive_read_open_filename_w(a, m_sourcePath.toStdWString().c_str(), 65536);
    else
        r = archive_read_open_memory(a, m_data.constData(), static_cast<size_t>(m_data.size()));
#else
    if (!m_sourcePath.isEmpty())
        r = archive_read_open_filename(a, QFile::encodeName(m_sourcePath).constData(), 65536);
    else
        r = archive_read_open_memory(a, m_data.constData(), static_cast<size_t>(m_data.size()));
#endif
    if (r != ARCHIVE_OK) {
        m_lastError = QString::fromUtf8(archive_error_string(a));
        if (m_lastError.isEmpty())
            m_lastError = QObject::tr("无法打开归档文件。");
        return false;
    }
    return true;
}

// 遍历条目公共部分：path/isDir/size/mtime 填充
ArchiveEntry ArchiveReader::entryFrom(struct archive_entry *entry) const
{
    // 文件名甄别（实测行为，本构建无 iconv）：
    //  - UTF-8(EFS) zip：libarchive 内部 utf8→本地字符集转换失败时
    //    pathname() 为 NULL，pathname_utf8() 是可靠 UTF-8；
    //  - GBK zip：pathname() 是原始 GBK 字节，pathname_utf8() 是把原始
    //    字节按 Latin-1 逐字节映射的乱码（字符全部落在 U+0080~U+00FF）。
    // 策略：pathname_utf8() 含 Latin-1 高位字符视为不可信，改用 pathname()
    // 原始字节走 UTF-8/GB18030 启发式（decodeName）。
    const char *u8 = archive_entry_pathname_utf8(entry);
    QString path;
    if (u8) {
        const QString u8str = QString::fromUtf8(u8);
        bool mojibake = false;
        for (const QChar &c : u8str) {
            if (c.unicode() >= 0x80 && c.unicode() <= 0xFF) {
                mojibake = true;
                break;
            }
        }
        if (!mojibake)
            path = u8str;
    }
    if (path.isEmpty())
        path = decodeName(archive_entry_pathname(entry));
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));

    ArchiveEntry e;
    e.path = path;
    e.isDir = (archive_entry_filetype(entry) == AE_IFDIR) || path.endsWith(QLatin1Char('/'));
    if (!e.isDir && archive_entry_size_is_set(entry) && archive_entry_size(entry) >= 0)
        e.size = static_cast<quint64>(archive_entry_size(entry));
    if (archive_entry_mtime_is_set(entry) && archive_entry_mtime(entry) > 0)
        e.mtime = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(archive_entry_mtime(entry)));
    return e;
}

bool ArchiveReader::open(const QString &filePath)
{
    close();
    m_cancel = false;
    if (!QFileInfo::exists(filePath)) {
        m_lastError = QObject::tr("文件不存在：%1").arg(filePath);
        return false;
    }
    m_sourcePath = filePath;

    struct archive *a = newReader();
    if (!openSource(a)) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry = nullptr;
    int r;
    while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
        if (r == ARCHIVE_FATAL) {
            m_lastError = QString::fromUtf8(archive_error_string(a));
            if (m_lastError.isEmpty())
                m_lastError = QObject::tr("归档文件已损坏或格式不受支持。");
            archive_read_free(a);
            m_sourcePath.clear();
            return false;
        }
        if (r < ARCHIVE_WARN) // ARCHIVE_FAILED：单条目头损坏，跳过继续
            continue;
        m_entries.append(entryFrom(entry));
    }

    const char *filter = archive_filter_name(a, 0);
    const char *format = archive_format_name(a);
    if (filter && format && strcmp(filter, "none") != 0)
        m_formatName = QStringLiteral("%1/%2").arg(QString::fromLatin1(filter), QString::fromLatin1(format));
    else if (format)
        m_formatName = QString::fromLatin1(format);
    else
        m_formatName = QStringLiteral("raw");

    archive_read_free(a);
    m_open = true;
    return true;
}

bool ArchiveReader::open(const QByteArray &data)
{
    close();
    m_cancel = false;
    m_data = data;

    struct archive *a = newReader();
    if (!openSource(a)) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry = nullptr;
    int r;
    while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
        if (r == ARCHIVE_FATAL) {
            m_lastError = QString::fromUtf8(archive_error_string(a));
            if (m_lastError.isEmpty())
                m_lastError = QObject::tr("归档文件已损坏或格式不受支持。");
            archive_read_free(a);
            return false;
        }
        if (r < ARCHIVE_WARN)
            continue;
        m_entries.append(entryFrom(entry));
    }

    const char *filter = archive_filter_name(a, 0);
    const char *format = archive_format_name(a);
    if (filter && format && strcmp(filter, "none") != 0)
        m_formatName = QStringLiteral("%1/%2").arg(QString::fromLatin1(filter), QString::fromLatin1(format));
    else if (format)
        m_formatName = QString::fromLatin1(format);

    archive_read_free(a);
    m_open = true;
    return true;
}

bool ArchiveReader::openFrom(const QString &path, const QByteArray &data)
{
    return path.isEmpty() ? open(data) : open(path);
}

quint64 ArchiveReader::totalSize() const
{
    quint64 sum = 0;
    for (const ArchiveEntry &e : m_entries)
        sum += e.size;
    return sum;
}

bool ArchiveReader::seekTo(struct archive *a, int index)
{
    struct archive_entry *entry = nullptr;
    int i = 0;
    int r;
    while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
        if (r == ARCHIVE_FATAL) {
            m_lastError = QString::fromUtf8(archive_error_string(a));
            return false;
        }
        if (i == index)
            return true;
        ++i;
    }
    m_lastError = QObject::tr("条目不存在或已被移除。");
    return false;
}

QString ArchiveReader::safeDestPath(const QString &destDir, const QString &entryPath) const
{
    // 防止 ".." 条目逃出解压目录（zip-slip）
    const QStringList parts = entryPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStringList cleaned;
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QLatin1String("."))
            continue;
        if (part == QLatin1String("..")) {
            // 与主流解压工具一致：向上越界直接截断到根
            cleaned.clear();
            continue;
        }
        cleaned.append(sanitizeComponent(part));
    }
    if (cleaned.isEmpty())
        return QString();
    return destDir + QLatin1Char('/') + cleaned.join(QLatin1Char('/'));
}

bool ArchiveReader::reportProgress(quint64 done, quint64 total, const QString &name)
{
    if (m_cancel)
        return false;
    if (m_progress)
        m_progress(done, total, name);
    return !m_cancel;
}

bool ArchiveReader::writeCurrentData(struct archive *a, const QString &destPath,
                                     quint64 baseDone, const QString &entryName)
{
    QFile out(destPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = QObject::tr("无法创建文件：%1").arg(destPath);
        return false;
    }
    char buf[65536];
    quint64 written = 0;
    for (;;) {
        if (m_cancel) {
            m_lastError = QObject::tr("已取消。");
            out.close();
            out.remove();
            return false;
        }
        const la_ssize_t n = archive_read_data(a, buf, sizeof(buf));
        if (n == 0)
            break;
        if (n < ARCHIVE_WARN) {
            m_lastError = QString::fromUtf8(archive_error_string(a));
            if (m_lastError.isEmpty())
                m_lastError = QObject::tr("解压数据失败（条目可能被加密或已损坏）。");
            out.close();
            out.remove();
            return false;
        }
        if (out.write(buf, n) != n) {
            m_lastError = QObject::tr("写入文件失败：%1").arg(destPath);
            out.close();
            out.remove();
            return false;
        }
        written += static_cast<quint64>(n);
        reportProgress(baseDone + written, m_progressTotalBytes, entryName);
    }
    out.close();
    return true;
}

QString ArchiveReader::extractEntry(int index, const QString &destDir)
{
    if (!m_open || index < 0 || index >= m_entries.size())
        return QString();

    const ArchiveEntry info = m_entries.at(index);
    const QString dest = safeDestPath(destDir, info.path);
    if (dest.isEmpty())
        return QString();

    // 单条目进度：总量即该条目大小
    m_progressTotalBytes = info.size;

    struct archive *a = newReader();
    bool ok = openSource(a) && seekTo(a, index);
    if (ok) {
        QDir().mkpath(QFileInfo(dest).absolutePath());
        ok = writeCurrentData(a, dest, 0, info.path);
    }
    archive_read_free(a);
    return ok ? dest : QString();
}

bool ArchiveReader::extractAll(const QString &destDir)
{
    if (!m_open)
        return false;

    // 整体进度：总字节 = 全部文件条目大小之和
    quint64 totalBytes = 0;
    for (const ArchiveEntry &e : m_entries)
        totalBytes += e.size;
    m_progressTotalBytes = totalBytes;

    struct archive *a = newReader();
    if (!openSource(a)) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry = nullptr;
    QStringList failed;
    quint64 doneBase = 0; // 已完成条目的累计字节
    int r;
    while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
        if (r == ARCHIVE_FATAL)
            break;
        if (m_cancel) {
            m_lastError = QObject::tr("已取消。");
            archive_read_free(a);
            return false;
        }
        if (r < ARCHIVE_WARN)
            continue;

        const ArchiveEntry e = entryFrom(entry);
        const QString dest = safeDestPath(destDir, e.path);
        if (dest.isEmpty())
            continue;

        if (e.isDir) {
            QDir().mkpath(dest);
            continue;
        }
        QDir().mkpath(QFileInfo(dest).absolutePath());
        if (!writeCurrentData(a, dest, doneBase, e.path))
            failed.append(e.path);
        doneBase += e.size;
    }
    archive_read_free(a);

    if (m_cancel) {
        m_lastError = QObject::tr("已取消。");
        return false;
    }
    if (!failed.isEmpty()) {
        m_lastError = QObject::tr("以下条目解压失败（可能加密或损坏）：%1")
                          .arg(failed.join(QStringLiteral("、")));
        return false;
    }
    return true;
}

QString ArchiveReader::extractToTemp(int index)
{
    if (!m_open || index < 0 || index >= m_entries.size())
        return QString();

    if (m_tempSessionDir.isEmpty()) {
        m_tempSessionDir = tempRoot() + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDir().mkpath(m_tempSessionDir);
    }
    return extractEntry(index, m_tempSessionDir);
}

QString ArchiveReader::tempRoot()
{
    static bool cleaned = false;
    const QString root = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                         + QStringLiteral("/UniFileHub/zip-preview");
    if (!cleaned) {
        cleaned = true;
        // 清理上次运行残留（预览临时文件无需跨会话保留）
        QDir(root).removeRecursively();
    }
    QDir().mkpath(root);
    return root;
}
