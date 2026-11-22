#ifndef ARCHIVEREADER_H
#define ARCHIVEREADER_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVector>

// 单条归档条目信息
struct ArchiveEntry
{
    QString path;            // 解码后的完整相对路径（'/' 分隔）
    quint64 size = 0;        // 原始大小
    QDateTime mtime;         // 修改时间（无效时为空）
    bool isDir = false;
};

// libarchive 封装：归档读取/解压（UI 无关）
// 支持 zip/tar/tar.gz/tar.bz2/tar.xz/tar.zst/7z/cab/iso/cpio 等
// libarchive 能识别的格式；加密条目可在列表中展示，解压时报错。
class ArchiveReader
{
public:
    ArchiveReader() = default;
    ~ArchiveReader();

    // 打开成功返回 true；失败原因见 lastError()
    bool open(const QString &filePath);
    bool open(const QByteArray &data);   // 云端/内存路径
    void close();
    bool isOpen() const { return m_open; }

    const QVector<ArchiveEntry> &entries() const { return m_entries; }
    quint64 totalSize() const;
    QString formatName() const { return m_formatName; }
    QString lastError() const { return m_lastError; }

    // 解压单个条目（index 对应 entries() 下标）到 destDir，保留内部目录结构；
    // 返回解压后的绝对路径，失败返回空串
    QString extractEntry(int index, const QString &destDir);

    // 解压全部条目到 destDir；有失败条目时返回 false 并写入 lastError
    bool extractAll(const QString &destDir);

    // 解压到本进程的归档预览临时目录，返回文件绝对路径，失败返回空串
    QString extractToTemp(int index);

    // 归档预览临时目录根；首次调用时清空上次运行残留
    static QString tempRoot();

private:
    QString decodeName(const char *raw) const;
    // 打开一个新的读取实例（供解压用），调用方负责 archive_read_free
    struct archive *newReader();
    // 用文件路径或内存数据打开读取实例
    bool openSource(struct archive *a);
    // 从 libarchive 条目提取公共信息
    ArchiveEntry entryFrom(struct archive_entry *entry) const;
    // 在新实例中跳到第 index 个条目
    bool seekTo(struct archive *a, int index);
    // 把当前条目数据写入 destPath
    bool writeCurrentData(struct archive *a, const QString &destPath);
    // 目标路径净化：防 zip-slip，处理 Windows 非法字符
    QString safeDestPath(const QString &destDir, const QString &entryPath) const;

    QString m_lastError;
    QString m_formatName;
    QString m_sourcePath;          // 文件模式来源路径（内存模式为空）
    QByteArray m_data;             // 内存模式数据保活
    QVector<ArchiveEntry> m_entries;
    QString m_tempSessionDir;      // 本实例的临时子目录（懒创建）
    bool m_open = false;
};

#endif // ARCHIVEREADER_H
