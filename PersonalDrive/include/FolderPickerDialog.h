#ifndef FOLDERPICKERDIALOG_H
#define FOLDERPICKERDIALOG_H

#include <QDialog>
#include <QJsonArray>

class DriveApiClient;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

// 供"移动"等功能选择网盘目标文件夹。
// 内部使用独立的 DriveApiClient 拉取目录树，避免干扰 DriveManager 的当前目录状态。
class FolderPickerDialog : public QDialog
{
    Q_OBJECT

public:
    // excludeFolderId：被移动的文件夹自身（连同其子树）不会出现在可选位置中，非文件夹传 -1
    explicit FolderPickerDialog(int excludeFolderId, QWidget *parent = nullptr);

    // 返回选中的目标文件夹 id，未选中时返回 -1（根目录为 0）
    int selectedFolderId() const;

private slots:
    void onItemExpanded(QTreeWidgetItem *item);
    void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onFolderListReceived(const QJsonArray &fileList);
    void onFetchError(const QString &errorMessage);

private:
    void requestChildren(QTreeWidgetItem *item);
    void updateOkState();
    QString folderPathOf(QTreeWidgetItem *item) const;

    DriveApiClient *m_client = nullptr;
    QTreeWidget *m_tree = nullptr;
    QLabel *m_pathLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_okButton = nullptr;
    int m_excludeFolderId = -1;
    QTreeWidgetItem *m_pendingItem = nullptr;
};

#endif // FOLDERPICKERDIALOG_H
