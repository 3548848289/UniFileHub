#include "include/DriveItem.h"

DriveItem::DriveItem(int id, const QString &name, int parentId, const QDateTime &createdAt)
    : m_id(id)
    , m_name(name)
    , m_parentId(parentId)
    , m_createdAt(createdAt)
{}

int DriveItem::getId() const
{
    return m_id;
}

QString DriveItem::getName() const
{
    return m_name;
}

void DriveItem::setName(const QString &name)
{
    m_name = name;
}

int DriveItem::getParentId() const
{
    return m_parentId;
}

void DriveItem::setParentId(int parentId)
{
    m_parentId = parentId;
}

QDateTime DriveItem::getCreatedAt() const
{
    return m_createdAt;
}