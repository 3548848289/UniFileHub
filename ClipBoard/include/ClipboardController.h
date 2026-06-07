#ifndef CLIPBOARDCONTROLLER_H
#define CLIPBOARDCONTROLLER_H

#include <QObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include "ClipboardHistoryManager.h"
#include "ClipboardItem/ClipboardItem.h"
#include "ClipboardMonitor.h"

class ClipboardCloudClient;
class ClipboardView;

class ClipboardController : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardController(QObject *parent = nullptr);
    ~ClipboardController() override;

    void setView(ClipboardView* view);
    ClipboardHistoryManager* getHistoryManager();

public slots:
    void handleNewClipboardItem(ClipboardItem* item);
    void copyItemToClipboard(ClipboardItem* item);
    void deleteItem(ClipboardItem* item);
    void clearAllItems();
    void pinItem(ClipboardItem* item);
    void saveHistory();
    void openFileLocation(ClipboardItem* item);
    void loadHistory(int hours);
    void searchItems(const QString& searchText);
    void syncItemToCloud(ClipboardItem* item);
    void unsyncItemFromCloud(ClipboardItem* item);
    void refreshCloudItems();

private slots:
    void onItemAdded(ClipboardItem* item);
    void onItemRemoved(ClipboardItem* item);
    void onItemsCleared();
    void onItemPinnedStatusChanged(ClipboardItem* item);
    void handleCloudItemsFetched(const QJsonArray &items);

signals:
    void itemAddedToModel(ClipboardItem* item);
    void itemRemovedFromModel(ClipboardItem* item);
    void modelCleared();
    void itemPinnedChanged(ClipboardItem* item);
    void infoMessageRequested(const QString &message);
    void errorMessageRequested(const QString &message);

private:
    ClipboardHistoryManager m_historyManager;
    ClipboardMonitor* m_clipboardMonitor;
    ClipboardView* m_view;
    ClipboardCloudClient* m_cloudClient;
};

#endif // CLIPBOARDCONTROLLER_H
