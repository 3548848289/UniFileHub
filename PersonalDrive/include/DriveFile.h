#ifndef DRIVEFILE_H
#define DRIVEFILE_H

#include "DriveItem.h"

class DriveFile : public DriveItem
{
public:
    DriveFile(int id, const QString &name, int parentId, qint64 size, const QString &mimeType, const QDateTime &createdAt);
    
    // 获取文件大小（字节）
    qint64 getSize() const;
    
    // 获取文件类型
    QString getMimeType() const;
    
    // 实现父类虚函数
    bool isFolder() const override;
    ItemType getType() const override;

private:
    qint64 m_size;
    QString m_mimeType;
};

#endif // DRIVEFILE_H