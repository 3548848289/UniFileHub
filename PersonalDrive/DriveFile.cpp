#include "include/DriveFile.h"

DriveFile::DriveFile(int id, const QString &name, int parentId, qint64 size, const QString &mimeType, const QDateTime &createdAt)
    : DriveItem(id, name, parentId, createdAt)
    , m_size(size)
    , m_mimeType(mimeType)
{}

qint64 DriveFile::getSize() const
{
    return m_size;
}

QString DriveFile::getMimeType() const
{
    return m_mimeType;
}

bool DriveFile::isFolder() const
{
    return false;
}

DriveItem::ItemType DriveFile::getType() const
{
    return File;
}