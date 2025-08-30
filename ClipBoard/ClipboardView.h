#ifndef CLIPBOARDVIEW_H
#define CLIPBOARDVIEW_H

#include <QWidget>
#include <QClipboard>
#include <QLabel>
#include <QListWidgetItem>
#include <QDialog>
#include <QScrollArea>
#include <memory>
#include <vector>
#include <QStringList>
#include "../manager/include/dbService.h"
#include "../Setting/include/SettingManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ClipboardView; }
QT_END_NAMESPACE

// 前向声明
class dbService;
class SettingManager;

// 剪贴板项基类 - 采用多态设计
class ClipboardItem {
public:
    enum Type { Text, Image, File };

    explicit ClipboardItem(Type type) : m_type(type) {}
    virtual ~ClipboardItem() = default;

    Type type() const { return m_type; }
    virtual QListWidgetItem* createListWidgetItem() const = 0;
    virtual void copyToClipboard(QClipboard* clipboard) const = 0;
    virtual QString serialize() const = 0;

protected:
    Type m_type;
};

// 文本类型剪贴板项
class TextClipboardItem : public ClipboardItem {
public:
    explicit TextClipboardItem(const QString& text)
        : ClipboardItem(Text), m_text(text) {}

    QListWidgetItem* createListWidgetItem() const override;
    void copyToClipboard(QClipboard* clipboard) const override;
    QString serialize() const override { return m_text; }

    const QString& text() const { return m_text; }

private:
    QString m_text;
};

// 图片类型剪贴板项
class ImageClipboardItem : public ClipboardItem {
public:
    explicit ImageClipboardItem(const QPixmap& pixmap);
    explicit ImageClipboardItem(const QByteArray& data);

    QListWidgetItem* createListWidgetItem() const override;
    void copyToClipboard(QClipboard* clipboard) const override;
    QString serialize() const override;

    const QPixmap& pixmap() const { return m_pixmap; }
    const QByteArray& data() const { return m_data; }

private:
    QPixmap m_pixmap;
    QByteArray m_data; // 存储为PNG格式
};

// 文件类型剪贴板项
class FileClipboardItem : public ClipboardItem {
public:
    explicit FileClipboardItem(const QStringList& filePaths);

    QListWidgetItem* createListWidgetItem() const override;
    void copyToClipboard(QClipboard* clipboard) const override;
    QString serialize() const override;

    const QStringList& filePaths() const { return m_filePaths; }
    bool isImageFile() const;
    static QStringList s_supportedImageFormats;

private:
    QStringList m_filePaths;

};

// 图片预览对话框
class ImagePreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImagePreviewDialog(const QPixmap& pixmap, QWidget *parent = nullptr);

private:
    QScrollArea* m_scrollArea;
    QLabel* m_imageLabel;
};

// 文件类型检测工具类
class FileTypeDetector {
public:
    static bool isImageFile(const QString& path);
    static QString getFileExtension(const QString& path);
    static QString toLocalPath(const QString& path);
    static bool fileExists(const QString& path);
};

// 主剪贴板视图类
class ClipboardView : public QWidget {
    Q_OBJECT

public:
    ClipboardView(QWidget *parent = nullptr);
    ~ClipboardView() override;

private slots:
    void onClipboardChanged();
    void on_clearButton_clicked();
    void on_saveButton_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_customContextMenuRequested(const QPoint &pos);

    void copyItem();
    void previewImage();
    void deleteItem();
    void openFileLocation();

private:
    Ui::ClipboardView *ui;
    QClipboard* m_clipboard;
    dbService& m_dbService;
    int m_initialItemCount;
    QListWidgetItem* m_currentRightClickedItem;
    std::vector<std::unique_ptr<ClipboardItem>> m_clipboardItems;

    void initializeListWidget();
    void loadHistory();
    void addClipboardItem(std::unique_ptr<ClipboardItem> item);
    ClipboardItem* findItemForListWidgetItem(QListWidgetItem* listItem);
};

#endif // CLIPBOARDVIEW_H

