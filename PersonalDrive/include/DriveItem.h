#ifndef DRIVEITEM_H
#define DRIVEITEM_H

#include <QString>
#include <QDateTime>

// 抽象基类：DriveItem
class DriveItem
{
public:
    enum ItemType {
        File,
        Folder
    };

    DriveItem(int id, const QString &name, int parentId, const QDateTime &createdAt);
    virtual ~DriveItem() = default;

    // 获取项目ID
    int getId() const;
    
    // 获取项目名称
    QString getName() const;
    void setName(const QString &name);
    
    // 获取父目录ID
    int getParentId() const;
    void setParentId(int parentId);
    
    // 获取创建时间
    QDateTime getCreatedAt() const;
    
    // 判断是否为文件夹
    virtual bool isFolder() const = 0;
    
    // 获取项目类型
    virtual ItemType getType() const = 0;

protected:
    int m_id;
    QString m_name;
    int m_parentId;
    QDateTime m_createdAt;
};

#endif // DRIVEITEM_H