#ifndef CLIPBOARDCONTROLLER_H
#define CLIPBOARDCONTROLLER_H

#include <QObject>
#include <QByteArray>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QMap>
#include <QSet>
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
    void clearCloudItems();
    void refreshCloudItems();
    // 一键把当前系统剪贴板内容（文本/图片）同步到云端
    void quickSyncCurrentClipboard();
    // 一键把云端最新记录复制到本机剪贴板
    void quickCopyLatestCloudItem();

private slots:
    void onItemAdded(ClipboardItem* item);
    void onItemRemoved(ClipboardItem* item);
    void onItemsCleared();
    void onItemPinnedStatusChanged(ClipboardItem* item);
    void handleCloudItemsFetched(const QJsonArray &items);
    void handleCloudImagesFetched(const QJsonArray &images);
    void handleImageDataReceived(int cloudImageId, const QByteArray &data);

signals:
    void itemAddedToModel(ClipboardItem* item);
    void itemRemovedFromModel(ClipboardItem* item);
    void modelCleared();
    void itemPinnedChanged(ClipboardItem* item);
    void infoMessageRequested(const QString &message);
    void errorMessageRequested(const QString &message);
    // 快速操作（快速同步/快速复制）的轻量提示，视图用悬浮条展示
    void quickActionMessage(const QString &message, bool isError);

private:
    bool isItemAlreadyInCloud(ClipboardItem* item) const;
    void startMissingImageDownloads();
    void tryRebuildCloudModel();
    void rebuildModelFromHistory();

    ClipboardHistoryManager m_historyManager;
    ClipboardMonitor* m_clipboardMonitor;
    ClipboardView* m_view;
    ClipboardCloudClient* m_cloudClient;

    // 云端图片二进制缓存（按云端 id），刷新时避免重复下载
    QMap<int, QByteArray> m_imageDataCache;
    QSet<int> m_pendingDownloadIds;
    QJsonArray m_pendingTextItems;
    QJsonArray m_pendingImageMeta;
    bool m_textFetchDone = false;
    bool m_imageFetchDone = false;
    bool m_refreshInFlight = false;
    bool m_refreshQueued = false;
    // 快速同步已发起上传，等 uploadSucceeded/uploadFailed 给出结果提示
    bool m_quickSyncPending = false;
};

#endif // CLIPBOARDCONTROLLER_H
