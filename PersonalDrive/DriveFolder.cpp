#include "include/DriveFolder.h"

DriveFolder::DriveFolder(int id, const QString &name, int parentId, const QDateTime &createdAt)
    : DriveItem(id, name, parentId, createdAt)
{}

bool DriveFolder::isFolder() const
{
    return true;
}

DriveItem::ItemType DriveFolder::getType() const
{
    return Folder;
}