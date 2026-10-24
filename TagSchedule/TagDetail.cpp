#include "include/TagDetail.h"
#include "ui/ui_TagDetail.h"
#include "../manager/include/DataChangeNotifier.h"

#include <QDateTimeEdit>
#include <QLineEdit>
#include <QPlainTextEdit>


TagDetail::TagDetail(QWidget *parent, FilePathInfo info) : QWidget(parent),
    ui(new Ui::TagDetail), dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    m_messagePopup = new InlineMessagePopup(this);
    m_messagePopup->setPanelWidget(this); // 弹窗显示在面板内部顶部居中
    init(info);
    registerInstance(info.filePath);
}

void TagDetail::init(FilePathInfo info) {
    if (!info.expirationDate.isValid()) {
        info.expirationDate = QDateTime::currentDateTime().addDays(1);
    }
    if (info.reminderType.isEmpty()) {
        info.reminderType = QStringLiteral("弹窗提醒"); // 默认提醒方式
    }
    fileInfo = info;
    oldFilePath = info.filePath;

    ui->tableWidget->setRowCount(5);
    ui->tableWidget->setColumnCount(2);

    auto readOnlyItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    // 设置表格项
    ui->tableWidget->setItem(0, 0, new QTableWidgetItem("文件路径"));
    ui->tableWidget->setItem(1, 0, new QTableWidgetItem("标签"));
    ui->tableWidget->setItem(2, 0, new QTableWidgetItem("过期时间"));
    ui->tableWidget->setItem(3, 0, new QTableWidgetItem("备注"));
    ui->tableWidget->setItem(4, 0, new QTableWidgetItem("文件状态"));

    ui->tableWidget->setItem(0, 1, readOnlyItem(info.filePath));

    // 标签、到期时间、备注用真正的编辑控件,确认后全量保存
    tagNameEdit = new QLineEdit(info.tagName);
    tagNameEdit->setPlaceholderText(QStringLiteral("标签名称"));
    ui->tableWidget->setCellWidget(1, 1, tagNameEdit);

    expirationEdit = new QDateTimeEdit(info.expirationDate);
    expirationEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    expirationEdit->setCalendarPopup(true);
    ui->tableWidget->setCellWidget(2, 1, expirationEdit);

    annotationEdit = new QPlainTextEdit(info.annotation);
    annotationEdit->setPlaceholderText(QStringLiteral("备注（可选）"));
    ui->tableWidget->setCellWidget(3, 1, annotationEdit);

    QFileInfo fileInfoCheck(info.filePath);
    ui->tableWidget->setItem(4, 1, readOnlyItem(fileInfoCheck.exists()
                                                    ? QStringLiteral("文件存在")
                                                    : QStringLiteral("文件不存在")));

    // 初始化提醒方式下拉框
    int index = ui->reminderTypeComboBox->findText(fileInfo.reminderType);
    if (index >= 0) {
        ui->reminderTypeComboBox->setCurrentIndex(index);
    }

    ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableWidget->horizontalScrollBar()->setSingleStep(5);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setVisible(false);
}

TagDetail::TagDetail(QWidget *parent, QString filePath) :
    QWidget(parent), ui(new Ui::TagDetail), dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    m_messagePopup = new InlineMessagePopup(this);
    m_messagePopup->setPanelWidget(this); // 弹窗显示在面板内部顶部居中
    fileInfo.filePath = filePath;
    // 没有标签记录时字段保持为空,由 init() 填充默认值,窗口仍可编辑保存
    dbservice.dbTags().getFileInfoByFilePath(filePath, fileInfo);
    init(fileInfo);
    registerInstance(fileInfo.filePath);
}

TagDetail::~TagDetail()
{
    if (!registryKey.isEmpty()) {
        auto &registry = instances();
        auto it = registry.find(registryKey);
        if (it != registry.end() && it.value() == this) {
            registry.erase(it);
        }
    }
    delete ui;
}

QHash<QString, TagDetail *> &TagDetail::instances()
{
    static QHash<QString, TagDetail *> registry;
    return registry;
}

void TagDetail::registerInstance(const QString &key)
{
    if (key.isEmpty()) {
        registryKey.clear();
        return;
    }

    auto &registry = instances();
    auto it = registry.find(key);
    if (it != registry.end() && it.value() != this) {
        // 同路径已有旧窗口时由新窗口接管注册,旧窗口关闭后不会误删新窗口的注册
        registry.erase(it);
    }
    registry.insert(key, this);
    registryKey = key;
}

void TagDetail::rekeyInstance(const QString &oldKey, const QString &newKey)
{
    auto &registry = instances();
    if (!oldKey.isEmpty()) {
        auto it = registry.find(oldKey);
        if (it != registry.end() && it.value() == this) {
            registry.erase(it);
        }
    }
    registryKey.clear();
    registerInstance(newKey);
}

TagDetail *TagDetail::showForFile(const FilePathInfo &fileInfo)
{
    auto &registry = instances();
    auto it = registry.find(fileInfo.filePath);
    if (it != registry.end()) {
        it.value()->show();
        it.value()->raise();
        it.value()->activateWindow();
        return it.value();
    }

    auto *detail = new TagDetail(nullptr, fileInfo);
    detail->setAttribute(Qt::WA_DeleteOnClose, true);
    detail->show();
    return detail;
}

TagDetail *TagDetail::showForPath(const QString &filePath)
{
    FilePathInfo info;
    info.filePath = filePath;
    return showForFile(info);
}

void TagDetail::on_choosePathBtn_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), ".", tr("所有文件 (*)"));
    if (!filePath.isEmpty()) {
        // 更新表格中的文件路径
        ui->tableWidget->setItem(0, 1, new QTableWidgetItem(filePath));
        // 更新fileInfo中的路径
        fileInfo.filePath = filePath;
    }
}

void TagDetail::on_YesBtn_clicked() {
    // 获取修改后的文件路径
    const QString newFilePath = ui->tableWidget->item(0, 1)->text().trimmed();
    if (newFilePath.isEmpty()) {
        m_messagePopup->showMessage(tr("文件路径不能为空！"), true);
        return;
    }

    // 检查文件是否存在
    if (!QFileInfo::exists(newFilePath)) {
        m_messagePopup->showMessage(tr("文件不存在！"), true);
        return;
    }

    const QString tagName = tagNameEdit->text().trimmed();
    if (tagName.isEmpty()) {
        m_messagePopup->showMessage(tr("标签名称不能为空！"), true);
        return;
    }

    const QString previousPath = oldFilePath;

    // 路径变了要先改路径:标签/备注/到期时间都是按路径定位记录的
    if (newFilePath != previousPath) {
        // 修复:updateFilePath 的签名是 (new, old),旧版传成了 (old, new),导致假成功
        if (!dbservice.dbTags().updateFilePath(newFilePath, previousPath)) {
            m_messagePopup->showMessage(tr("文件路径更新失败！"), true);
            return;
        }
    }

    // 保留提醒调度字段(lastReminderIndex 等),只更新窗口里可编辑的内容
    FilePathInfo updated = fileInfo;
    updated.filePath = newFilePath;
    updated.tagName = tagName;
    updated.expirationDate = expirationEdit->dateTime();
    updated.annotation = annotationEdit->toPlainText();
    updated.reminderType = ui->reminderTypeComboBox->currentText();

    if (!dbservice.dbTags().updateFileInfo(updated)) {
        m_messagePopup->showMessage(tr("标签信息保存失败！"), true);
        if (newFilePath != previousPath) {
            // 信息没存上,把路径改回去保持一致
            dbservice.dbTags().updateFilePath(previousPath, newFilePath);
        }
        return;
    }

    fileInfo = updated;

    if (newFilePath != previousPath) {
        rekeyInstance(previousPath, newFilePath);
        oldFilePath = newFilePath;
        auto *pathItem = new QTableWidgetItem(newFilePath);
        pathItem->setFlags(pathItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(0, 1, pathItem);
        ui->tableWidget->setItem(4, 1, new QTableWidgetItem(QStringLiteral("文件存在")));
    }

    m_messagePopup->showMessage(tr("标签已保存"));
    // 通知文件列表和备忘标签页自动刷新
    DataChangeNotifier::Instance().notifyTagChanged(newFilePath);
}
