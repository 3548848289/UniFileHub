#ifndef DRIVEFOLDER_H
#define DRIVEFOLDER_H

#include "DriveItem.h"

class DriveFolder : public DriveItem
{
public:
    DriveFolder(int id, const QString &name, int parentId, const QDateTime &createdAt);
    
    // 实现父类虚函数
    bool isFolder() const override;
    ItemType getType() const override;
};

#endif // DRIVEFOLDER_H