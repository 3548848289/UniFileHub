#include "include/FolderPickerDialog.h"
#include "include/DriveApiClient.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

constexpr int kItemIdRole = Qt::UserRole;      // 文件夹 id（占位节点为 -1）
constexpr int kLoadedRole = Qt::UserRole + 1;  // 子目录是否已加载

QTreeWidgetItem *createPlaceholderItem()
{
    auto *placeholder = new QTreeWidgetItem(QStringList(QStringLiteral("加载中...")));
    placeholder->setData(0, kItemIdRole, -1);
    placeholder->setDisabled(true);
    return placeholder;
}

void removePlaceholderChildren(QTreeWidgetItem *item)
{
    for (int i = item->childCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *child = item->child(i);
        if (child && child->data(0, kItemIdRole).toInt() == -1) {
            item->removeChild(child);
        }
    }
}

} // namespace

FolderPickerDialog::FolderPickerDialog(int excludeFolderId, QWidget *parent)
    : QDialog(parent),
      m_excludeFolderId(excludeFolderId)
{
    setWindowTitle(QStringLiteral("移动到..."));
    setModal(true);
    setMinimumSize(420, 480);

    m_client = new DriveApiClient(this);
    connect(m_client, &DriveApiClient::fileListReceived,
            this, &FolderPickerDialog::onFolderListReceived);
    connect(m_client, &DriveApiClient::fileListError,
            this, &FolderPickerDialog::onFetchError);

    auto *title = new QLabel(QStringLiteral("选择移动目标位置"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    title->setFont(titleFont);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabel(QStringLiteral("文件夹"));
    m_tree->header()->setStretchLastSection(true);
    connect(m_tree, &QTreeWidget::itemExpanded,
            this, &FolderPickerDialog::onItemExpanded);
    connect(m_tree, &QTreeWidget::currentItemChanged,
            this, &FolderPickerDialog::onCurrentItemChanged);

    m_pathLabel = new QLabel(this);
    m_pathLabel->setObjectName(QStringLiteral("pathLabel"));

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    m_messageLabel->setWordWrap(true);
    m_messageLabel->hide();

    m_okButton = new QPushButton(QStringLiteral("移动到这里"), this);
    m_okButton->setObjectName(QStringLiteral("okButton"));
    m_okButton->setCursor(Qt::PointingHandCursor);
    m_okButton->setMinimumHeight(34);
    m_okButton->setDefault(true);

    auto *cancelButton = new QPushButton(QStringLiteral("取消"), this);
    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setMinimumHeight(34);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(m_okButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(m_tree, 1);
    layout->addWidget(m_pathLabel);
    layout->addWidget(m_messageLabel);
    layout->addLayout(buttonLayout);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #ffffff; }"
        "QLabel { color: #1f2937; font-size: 14px; }"
        "QLabel#pathLabel { color: #6b7280; }"
        "QLabel#messageLabel { color: #b91c1c; }"
        "QTreeWidget { border: 1px solid #e5e7eb; border-radius: 6px; background: #ffffff; color: #1f2937; font-size: 14px; }"
        "QTreeWidget::item { height: 26px; }"
        "QTreeWidget::item:selected { background: #eff6ff; color: #1d4ed8; }"
        "QPushButton { border: 1px solid #d1d5db; border-radius: 6px; background: #ffffff; color: #374151; padding: 7px 16px; font-weight: 600; }"
        "QPushButton:hover { background: #f3f4f6; }"
        "QPushButton:disabled { color: #9ca3af; background: #f9fafb; }"
        "QPushButton#okButton { background: #2563eb; border-color: #2563eb; color: #ffffff; }"
        "QPushButton#okButton:hover { background: #1d4ed8; }"
        "QPushButton#okButton:disabled { background: #93c5fd; border-color: #93c5fd; color: #eff6ff; }"
    ));

    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // 根目录（id=0）
    auto *root = new QTreeWidgetItem(QStringList(QStringLiteral("个人云盘")));
    root->setData(0, kItemIdRole, 0);
    root->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    m_tree->addTopLevelItem(root);
    m_tree->setCurrentItem(root);
    updateOkState();

    requestChildren(root);
}

int FolderPickerDialog::selectedFolderId() const
{
    QTreeWidgetItem *current = m_tree->currentItem();
    if (!current) {
        return -1;
    }
    return current->data(0, kItemIdRole).toInt();
}

void FolderPickerDialog::onItemExpanded(QTreeWidgetItem *item)
{
    requestChildren(item);
}

void FolderPickerDialog::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    m_pathLabel->setText(QStringLiteral("移动到：%1").arg(current ? folderPathOf(current) : QString()));
    updateOkState();
}

void FolderPickerDialog::onFolderListReceived(const QJsonArray &fileList)
{
    QTreeWidgetItem *item = m_pendingItem;
    m_pendingItem = nullptr;
    if (!item) {
        return;
    }

    removePlaceholderChildren(item);

    int folderCount = 0;
    for (const QJsonValue &value : fileList) {
        const QJsonObject obj = value.toObject();
        if (!obj.value(QStringLiteral("is_dir")).toBool()) {
            continue;
        }

        const int id = obj.value(QStringLiteral("id")).toInt();
        if (id == m_excludeFolderId) {
            continue;
        }

        auto *child = new QTreeWidgetItem(QStringList(obj.value(QStringLiteral("name")).toString()));
        child->setData(0, kItemIdRole, id);
        child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        item->addChild(child);
        ++folderCount;
    }

    // 空目录不再显示展开箭头
    item->setChildIndicatorPolicy(folderCount > 0
                                      ? QTreeWidgetItem::ShowIndicator
                                      : QTreeWidgetItem::DontShowIndicator);
    item->setData(0, kLoadedRole, true);
    updateOkState();
}

void FolderPickerDialog::onFetchError(const QString &errorMessage)
{
    QTreeWidgetItem *item = m_pendingItem;
    m_pendingItem = nullptr;
    if (item) {
        removePlaceholderChildren(item);
    }

    m_messageLabel->setText(QStringLiteral("获取目录失败：%1").arg(errorMessage));
    m_messageLabel->show();
    updateOkState();
}

void FolderPickerDialog::requestChildren(QTreeWidgetItem *item)
{
    if (!item || m_pendingItem) {
        return;
    }
    if (item->data(0, kLoadedRole).toBool()) {
        return;
    }

    item->addChild(createPlaceholderItem());
    m_pendingItem = item;
    m_messageLabel->hide();
    m_client->getFileList(item->data(0, kItemIdRole).toInt());
}

void FolderPickerDialog::updateOkState()
{
    m_okButton->setEnabled(m_tree->currentItem() != nullptr);
}

QString FolderPickerDialog::folderPathOf(QTreeWidgetItem *item) const
{
    QStringList names;
    for (QTreeWidgetItem *node = item; node; node = node->parent()) {
        names.prepend(node->text(0));
    }
    return names.join(QStringLiteral(" / "));
}
