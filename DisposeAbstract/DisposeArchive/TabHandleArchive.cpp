#include "TabHandleArchive.h"
#include <functional>
#include <QDrag>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QMimeData>
#include <QSplitter>
#include <QStyle>
#include <QUrl>
#include <QApplication>
#include "../../Setting/include/SettingManager.h"

namespace {
constexpr int kUserRoleEntryIndex = Qt::UserRole; // 文件节点存条目下标，目录节点存 -1
}

// 树控件子类：启用拖拽出条目（按住已选中项拖动 = 解压到临时目录并以
// file:// URL 交给系统）。拖拽启用后视图自带的"长按滑动多选"不再触发，
// Ctrl+点选 / Shift+范围选不受影响。
class ArchiveTreeWidget : public QTreeWidget
{
public:
    using QTreeWidget::QTreeWidget;

    // 由 TabHandleArchive 注入：把条目下标解压到临时目录，返回落盘路径
    std::function<QStringList(const QList<int> &)> extractToTempFiles;

protected:
    void startDrag(Qt::DropActions supportedActions) override
    {
        QList<int> indexes;
        for (QTreeWidgetItem *item : selectedItems())
            collectFileIndexes(item, indexes);
        if (indexes.isEmpty())
            return;

        const QStringList paths = extractToTempFiles ? extractToTempFiles(indexes)
                                                     : QStringList();
        if (paths.isEmpty())
            return;

        auto *mimeData = new QMimeData;
        QList<QUrl> urls;
        for (const QString &p : paths)
            urls.append(QUrl::fromLocalFile(p));
        mimeData->setUrls(urls);

        QDrag drag(this);
        drag.setMimeData(mimeData);
        for (QTreeWidgetItem *item : selectedItems()) {
            if (item->data(0, kUserRoleEntryIndex).toInt() >= 0) {
                drag.setPixmap(item->icon(0).pixmap(32, 32));
                break;
            }
        }
        drag.exec(supportedActions & Qt::CopyAction ? Qt::CopyAction
                                                    : supportedActions);
    }

private:
    static void collectFileIndexes(QTreeWidgetItem *item, QList<int> &out)
    {
        const int index = item->data(0, kUserRoleEntryIndex).toInt();
        if (index >= 0)
            out.append(index);
        for (int i = 0; i < item->childCount(); ++i)
            collectFileIndexes(item->child(i), out);
    }
};

TabHandleArchive::TabHandleArchive(const QString &filePath, QWidget *parent)
    : TabAbstract(filePath, parent)
{
    buildUi();
}

void TabHandleArchive::buildUi()
{
    // 字体与文件树保持一致（12pt）
    QFont uiFont = font();
    uiFont.setPointSize(12);
    setFont(uiFont);

    // 工具栏由独立类管控；可见性跟随设置页"压缩包文件"开关（滑入滑出）
    m_controlBar = new ControlWidArchive(this);
    m_controlBar->setVisible(SettingManager::Instance().file_see_archive());
    connect(&SettingManager::Instance(), &SettingManager::settingChanged,
            this, [this](const QString &key, const QVariant &value) {
                if (key == QStringLiteral("file_see/archive")) {
                    m_controlBar->setVisible(value.toBool());
                }
            });

    m_tree = new ArchiveTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({tr("名称"), tr("大小"), tr("修改时间")});
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setUniformRowHeights(true);
    m_tree->header()->setStretchLastSection(false); // 列宽由 redistributeColumns 按 3:1:1 分配
    // 拖拽出条目：按住已选中项拖动即拖出（拖拽启用后长按滑动多选自动失效）
    m_tree->setDragEnabled(true);
    m_tree->setDefaultDropAction(Qt::CopyAction);
    m_tree->extractToTempFiles = [this](const QList<int> &indexes) {
        QStringList paths;
        for (int idx : indexes) {
            const QString p = m_reader.extractToTemp(idx);
            if (!p.isEmpty())
                paths.append(p);
        }
        return paths;
    };
    // 视口尺寸变化（首次显示/窗口拖动/布局变化）时重算列宽；
    // 不用页面 resizeEvent —— 那里拿到的 viewport 宽度是滞后一拍的旧值
    m_tree->viewport()->installEventFilter(this);

    // 中间文件树 + 底部操作栏（属性在上/按钮在下），可拖动分隔条（参考 TabHandleTXT：内容 700 / 操作栏 100）
    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_tree);
    splitter->addWidget(m_controlBar);
    splitter->setSizes({700, 100});

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    connect(m_controlBar, &ControlWidArchive::extractAllHereRequested, this, &TabHandleArchive::extractAllHere);
    connect(m_controlBar, &ControlWidArchive::extractAllToRequested, this, &TabHandleArchive::extractAllTo);
    connect(m_controlBar, &ControlWidArchive::extractSelectedToRequested, this, &TabHandleArchive::extractSelectedTo);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        previewEntry(item);
    });
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        bool hasFile = false;
        const auto selected = m_tree->selectedItems();
        for (QTreeWidgetItem *item : selected) {
            if (item->data(0, kUserRoleEntryIndex).toInt() >= 0) {
                hasFile = true;
                break;
            }
        }
        m_controlBar->setSelectionExtractable(hasFile);
    });
}

QString TabHandleArchive::humanSize(quint64 bytes) const
{
    if (bytes < 1024)
        return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024ULL * 1024 * 1024)
        return QStringLiteral("%1 MB").arg(bytes / 1024.0 / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2);
}

void TabHandleArchive::buildTree()
{
    m_tree->setSortingEnabled(false);
    m_tree->clear();

    QStyle *appStyle = style();
    const QIcon dirIcon = appStyle->standardIcon(QStyle::SP_DirIcon);
    const QIcon fileIcon = appStyle->standardIcon(QStyle::SP_FileIcon);

    // 按目录路径分组建树；未显式出现的父目录自动补齐
    QMap<QString, QTreeWidgetItem *> dirNodes;
    dirNodes.insert(QString(), nullptr); // 根

    // 创建节点：parent 为空时必须挂到 m_tree 上，
    // new QTreeWidgetItem(nullptr) 产生的是不显示的孤儿节点
    auto makeNode = [this](QTreeWidgetItem *parent) -> QTreeWidgetItem * {
        return parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_tree);
    };

    // lambda 无法自递归，用 std::function
    std::function<QTreeWidgetItem *(const QString &)> ensureDirNode =
        [this, &dirNodes, &dirIcon, &ensureDirNode, &makeNode](const QString &dirPath) -> QTreeWidgetItem * {
        if (dirPath.isEmpty())
            return nullptr;
        auto it = dirNodes.find(dirPath);
        if (it != dirNodes.end() && it.value())
            return it.value();

        const int slash = dirPath.lastIndexOf(QLatin1Char('/'));
        const QString parentPath = (slash >= 0) ? dirPath.left(slash) : QString();
        const QString name = (slash >= 0) ? dirPath.mid(slash + 1) : dirPath;
        QTreeWidgetItem *parent = ensureDirNode(parentPath);

        auto *node = makeNode(parent);
        node->setText(0, name);
        node->setIcon(0, dirIcon);
        node->setData(0, kUserRoleEntryIndex, -1);
        dirNodes.insert(dirPath, node);
        return node;
    };

    const auto &entries = m_reader.entries();
    for (int i = 0; i < entries.size(); ++i) {
        const ArchiveEntry &e = entries.at(i);
        QString path = e.path;
        if (path.endsWith(QLatin1Char('/')))
            path.chop(1);

        if (e.isDir) {
            ensureDirNode(path);
            continue;
        }

        const int slash = path.lastIndexOf(QLatin1Char('/'));
        const QString parentPath = (slash >= 0) ? path.left(slash) : QString();
        QTreeWidgetItem *parent = ensureDirNode(parentPath);

        auto *node = makeNode(parent);
        node->setText(0, (slash >= 0) ? path.mid(slash + 1) : path);
        node->setIcon(0, fileIcon);
        node->setText(1, humanSize(e.size));
        node->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        node->setText(2, e.mtime.isValid() ? e.mtime.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QStringLiteral("-"));
        node->setData(0, kUserRoleEntryIndex, i);
    }

    m_tree->expandAll();
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(0, Qt::AscendingOrder);

    quint64 total = m_reader.totalSize();
    m_controlBar->setStatusText(tr("格式：%1    条目：%2    总大小：%3")
                                .arg(m_reader.formatName())
                                .arg(entries.size())
                                .arg(humanSize(total)));
}

// 列宽按 3:1:1 分配（名称:大小:修改时间），占满窗口宽度
void TabHandleArchive::redistributeColumns()
{
    const int w = m_tree->viewport()->width();
    if (w <= 0)
        return;
    const int w0 = w * 3 / 5;
    const int w1 = w / 5;
    m_tree->setColumnWidth(0, w0);
    m_tree->setColumnWidth(1, w1);
    m_tree->setColumnWidth(2, w - w0 - w1);
}

bool TabHandleArchive::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tree->viewport() && event->type() == QEvent::Resize)
        redistributeColumns();
    return TabAbstract::eventFilter(watched, event);
}

void TabHandleArchive::collectFileIndexes(QTreeWidgetItem *item, QList<int> &out) const
{
    const int index = item->data(0, kUserRoleEntryIndex).toInt();
    if (index >= 0)
        out.append(index);
    for (int i = 0; i < item->childCount(); ++i)
        collectFileIndexes(item->child(i), out);
}

void TabHandleArchive::loadFromFile(const QString &fileName)
{
    setCurrentFilePath(fileName);
    m_rawData.clear(); // 文件模式按需从磁盘复制，避免整包占用内存

    if (!m_reader.open(fileName)) {
        QMessageBox::warning(this, tr("错误"),
                             tr("无法打开归档文件：%1").arg(m_reader.lastError()));
        return;
    }
    buildTree();
}

void TabHandleArchive::loadFromInternet(const QByteArray &content)
{
    m_rawData = content;
    if (!m_reader.open(content)) {
        QMessageBox::warning(this, tr("错误"),
                             tr("无法打开归档文件：%1").arg(m_reader.lastError()));
        return;
    }
    buildTree();
}

void TabHandleArchive::previewEntry(QTreeWidgetItem *item)
{
    if (!item)
        return;
    const int index = item->data(0, kUserRoleEntryIndex).toInt();
    if (index < 0)
        return; // 目录节点，交给默认展开/折叠

    const QString tempPath = m_reader.extractToTemp(index);
    if (tempPath.isEmpty()) {
        QMessageBox::warning(this, tr("预览失败"),
                             tr("%1\n\n%2").arg(item->text(0), m_reader.lastError()));
        return;
    }
    emit openFileRequested(tempPath);
}

void TabHandleArchive::extractAllHere()
{
    const QFileInfo info(currentFilePath);
    const QString destDir = info.absolutePath() + QLatin1Char('/') + info.completeBaseName();
    if (!QDir().mkpath(destDir))
        return;
    if (m_reader.extractAll(destDir))
        QMessageBox::information(this, tr("解压完成"), tr("已解压到：%1").arg(destDir));
    else
        QMessageBox::warning(this, tr("解压失败"), m_reader.lastError());
}

void TabHandleArchive::extractAllTo()
{
    const QString destDir = QFileDialog::getExistingDirectory(this, tr("选择解压目录"));
    if (destDir.isEmpty())
        return;
    if (m_reader.extractAll(destDir))
        QMessageBox::information(this, tr("解压完成"), tr("已解压到：%1").arg(destDir));
    else
        QMessageBox::warning(this, tr("解压失败"), m_reader.lastError());
}

void TabHandleArchive::extractSelectedTo()
{
    QList<int> indexes;
    const auto selected = m_tree->selectedItems();
    for (QTreeWidgetItem *item : selected)
        collectFileIndexes(item, indexes);
    if (indexes.isEmpty())
        return;

    const QString destDir = QFileDialog::getExistingDirectory(this, tr("选择解压目录"));
    if (destDir.isEmpty())
        return;

    QStringList failed;
    for (int index : indexes) {
        if (m_reader.extractEntry(index, destDir).isEmpty())
            failed.append(m_reader.entries().at(index).path);
    }
    if (failed.isEmpty())
        QMessageBox::information(this, tr("解压完成"), tr("已解压 %1 个文件到：%2").arg(indexes.size()).arg(destDir));
    else
        QMessageBox::warning(this, tr("解压失败"),
                             tr("以下条目失败：\n%1\n\n%2").arg(failed.join(QStringLiteral("\n")), m_reader.lastError()));
}

void TabHandleArchive::setContent(const QString &text)
{
    Q_UNUSED(text);
    QMessageBox::information(this, tr("不支持"), tr("压缩包为预览视图，不支持直接文本编辑。"));
}

QString TabHandleArchive::getContent() const
{
    QStringList lines;
    const auto &entries = m_reader.entries();
    for (const ArchiveEntry &e : entries) {
        if (!e.isDir)
            lines.append(QStringLiteral("%1\t%2").arg(e.path).arg(e.size));
    }
    return lines.join(QLatin1Char('\n'));
}

void TabHandleArchive::saveToFile(const QString &fileName)
{
    // 预览只读：云端模式原样保存字节；本地模式直接复制源文件
    if (m_rawData.isEmpty()) {
        if (currentFilePath.isEmpty() || !QFile::exists(currentFilePath)) {
            QMessageBox::warning(this, tr("错误"), tr("没有可保存的内容。"));
            return;
        }
        QFile::remove(fileName);
        if (!QFile::copy(currentFilePath, fileName)) {
            QMessageBox::warning(this, tr("错误"), tr("无法保存文件：%1").arg(fileName));
            return;
        }
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("错误"), tr("无法保存文件：%1").arg(fileName));
        return;
    }
    file.write(m_rawData);
    file.close();
}

void TabHandleArchive::ControlWidget(bool judge)
{
    isShowControl = judge; // 与 TXT/XLSX 保持一致的工具栏显隐标记
}
