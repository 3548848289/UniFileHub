#include "include/DataChangeNotifier.h"

DataChangeNotifier &DataChangeNotifier::Instance()
{
    static DataChangeNotifier instance;
    return instance;
}

void DataChangeNotifier::notifyBackupRecordAdded(const QString &filePath)
{
    emit backupRecordAdded(filePath);
}

void DataChangeNotifier::notifyTagChanged(const QString &filePath)
{
    emit tagChanged(filePath);
}
