#ifndef TABHANDLEARCHIVE_H
#define TABHANDLEARCHIVE_H

#include <QtGlobal>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include "../../main/include/TabAbstract.h"
#include "ArchiveReader.h"
#include "ControlWidArchive.h"

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

    QTreeWidget *m_tree = nullptr;
    ControlWidArchive *m_controlBar = nullptr;
    ArchiveReader m_reader;
    QByteArray m_rawData;
};

#endif // TABHANDLEARCHIVE_H
