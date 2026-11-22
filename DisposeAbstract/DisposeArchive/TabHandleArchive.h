#ifndef TABHANDLEARCHIVE_H
#define TABHANDLEARCHIVE_H

#include <QtGlobal>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <memory>
#include "../../main/include/TabAbstract.h"
#include "ArchiveReader.h"
#include "ControlWidArchive.h"

class ArchiveTreeWidget;

// 归档文件预览标签页（zip/tar/tgz/tbz/txz/7z/cab/iso/cpio 等）
class TabHandleArchive : public TabAbstract
{
    Q_OBJECT

public:
    explicit TabHandleArchive(const QString &filePath, QWidget *parent = nullptr);

    void setContent(const QString &text) override;
    QString getContent() const override;
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override;
    void saveToFile(const QString &fileName) override;
    void ControlWidget(bool judge) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildUi();
    void buildTree();
    void redistributeColumns();
    QString humanSize(quint64 bytes) const;
    void collectFileIndexes(QTreeWidgetItem *item, QList<int> &out) const;
    void extractAllHere();
    void extractAllTo();
    void extractSelectedTo();
    void previewEntry(QTreeWidgetItem *item);
    // 后台解压编排：task 在工作线程用独立 reader 实例执行，
    // 完成后 onDone 在 UI 线程回调；进度/取消走 m_activeReader
    void startExtractTask(const QString &busyText,
                          const std::function<bool(ArchiveReader &)> &task,
                          const std::function<void(bool ok, bool cancelled, const QString &error)> &onDone);

    ArchiveTreeWidget *m_tree = nullptr;
    ControlWidArchive *m_controlBar = nullptr;
    ArchiveReader m_reader;          // UI 线程：列表/解码用
    QByteArray m_rawData;
    bool m_extracting = false;                       // 是否有解压任务进行中
    std::shared_ptr<ArchiveReader> m_activeReader;   // 进行中任务的 reader（取消入口）
};

#endif // TABHANDLEARCHIVE_H
