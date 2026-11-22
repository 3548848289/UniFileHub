#ifndef ARCHIVEWRITER_H
#define ARCHIVEWRITER_H

#include <QString>
#include <functional>

// libarchive 归档写入器：把文件/文件夹打包为 .zip（deflate，UTF-8 文件名）
class ArchiveWriter
{
public:
    // progress(doneBytes, totalBytes, currentName) 按字节上报（写入循环逐块调用）；
    // 返回 false = 请求取消（取消后自动删除半成品）
    using ProgressCallback = std::function<bool(quint64, quint64, const QString &)>;

    // 把 paths 中的文件/文件夹压缩为 destZipPath（zip 根包含路径本身）；
    // 失败/取消返回 false 并写 error；失败时半成品 zip 会被删除
    static bool zipPaths(const QStringList &paths, const QString &destZipPath,
                         const ProgressCallback &progress, QString *error);
};

#endif // ARCHIVEWRITER_H
