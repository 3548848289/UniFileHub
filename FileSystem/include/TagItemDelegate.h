#ifndef TAGITEMDELEGATE_H
#define TAGITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QStyleOptionButton>
#include <QMouseEvent>
#include <QStyle>
#include <QFileSystemModel>
#include <QMenu>
#include <QAction>
#include <QHash>
#include <QVector>
#include <QClipboard>
#include <QProcess>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileInfo>
#include <QColor>

#include "../../TagSchedule/include/TagDetail.h"
#include "../../manager/include/dbService.h"
#include "../../manager/include/ServerManager.h"
#include "../../FileBackup/include/FileBackupWid.h"
#include "../../TagSchedule/include/AddTag.h"
class TagItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

signals:
    void tagbutClicked(const QModelIndex &index);
    void subbutClicked(const QModelIndex &index);
    void openFileRequested(const QString &filePath);
    void deleteFileRequested(const QString &filePath);
    void uploadToDriveRequested(const QString &filePath);
    void openTerminalRequested(const QString &directoryPath);
    void onFilesListUpdated(const QStringList& files);
    void TagUpdated();

public:
    mutable bool isButtonClicked = false;
    explicit TagItemDelegate(QObject *parent = nullptr, ServerManager *serverManager = nullptr);
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view,
                   const QStyleOptionViewItem &option, const QModelIndex &index) override;

    void showContextMenu(const QPoint &pos, const QModelIndex &index, QAbstractItemModel *model);

public slots:
    void onTagDeleted(const QString &filePath); // 清空标签缓存并刷新视图

private slots:
    void onNewTagTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onOpenFileTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onDeleteFileTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onRenameTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onCommitTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onUploadToDriveTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onCopyPathTriggered(QAbstractItemModel *model, const QModelIndex &index);
    void onOpenInExplorer(QAbstractItemModel *model, const QModelIndex &index);
    void onOpenInTerminal(QAbstractItemModel *model, const QModelIndex &index);
private:
    struct TagChip
    {
        QString text;
        QColor color;
        QRect rect;
    };

    QStringList fileList;

    ServerManager *serverManager;

    // 路径 -> 标签信息(无记录时 tagName 为空),以及路径 -> 是否有备份角标
    mutable QHash<QString, FilePathInfo> m_infoCache;
    mutable QHash<QString, bool> m_submissionsCache;

    const FilePathInfo &tagInfoFor(const QString &filePath) const;
    bool hasSubmissions(const QString &filePath) const;
    QVector<TagChip> layoutChips(const QStyleOptionViewItem &option,
                                 const FilePathInfo &info, bool hasSubmissionIcon) const;
    static QColor tagColor(const QString &tagName);

    void addTag(const QAbstractItemModel *model, const QModelIndex &index, AddTag &tagDialog);
    void cleanupTagRecordsForRemovedPath(const QString &removedPath); // 文件/目录删除后清理其标签记录

    dbService& dbservice;
};

#endif // TAGITEMDELEGATE_H
