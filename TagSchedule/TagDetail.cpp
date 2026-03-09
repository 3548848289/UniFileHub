#include "include/TagDetail.h"
#include "ui/ui_TagDetail.h"


TagDetail::TagDetail(QWidget *parent, FilePathInfo fileInfo) : QWidget(parent), fileInfo(fileInfo),
    ui(new Ui::TagDetail), dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    init(fileInfo);
    qDebug() << "TagDetail" << fileInfo.filePath;

}

void TagDetail::init(FilePathInfo fileInfo) {

    ui->tableWidget->setRowCount(5);
    ui->tableWidget->setColumnCount(2);

    // 设置表格项
    ui->tableWidget->setItem(0, 0, new QTableWidgetItem("文件路径"));
    ui->tableWidget->setItem(1, 0, new QTableWidgetItem("标签"));
    ui->tableWidget->setItem(2, 0, new QTableWidgetItem("过期时间"));
    ui->tableWidget->setItem(3, 0, new QTableWidgetItem("备注"));
    ui->tableWidget->setItem(4, 0, new QTableWidgetItem("文件状态"));

    ui->tableWidget->setItem(0, 1, new QTableWidgetItem(fileInfo.filePath));
    QTableWidgetItem *tagItem = new QTableWidgetItem(fileInfo.tagName);
    ui->tableWidget->setItem(1, 1, tagItem);
    ui->tableWidget->setItem(2, 1, new QTableWidgetItem(fileInfo.expirationDate.toString("yyyy-MM-ddTHH:mm:ss.zzz")));
    ui->tableWidget->setItem(3, 1, new QTableWidgetItem(fileInfo.annotation));
    
    // 保存原始文件路径
    oldFilePath = fileInfo.filePath;
    
    // 初始化提醒方式下拉框
    if (fileInfo.reminderType.isEmpty()) {
        fileInfo.reminderType = "弹窗提醒"; // 默认提醒方式
    }
    int index = ui->reminderTypeComboBox->findText(fileInfo.reminderType);
    if (index >= 0) {
        ui->reminderTypeComboBox->setCurrentIndex(index);
    }

    QFileInfo fileInfoCheck(fileInfo.filePath);
    if (!fileInfoCheck.exists()) {
        ui->tableWidget->setItem(4, 1, new QTableWidgetItem("文件不存在"));
    } else {
        ui->tableWidget->setItem(4, 1, new QTableWidgetItem("文件存在"));
    }
    ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableWidget->horizontalScrollBar()->setSingleStep(5);

    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setVisible(false);

}


TagDetail::TagDetail(QWidget *parent, QString filePath) :
    QWidget(parent), ui(new Ui::TagDetail), dbservice(dbService::instance("./SmartDesk.db")) {
    ui->setupUi(this);
    this->fileInfo.filePath = filePath; // 正确初始化filePath
    if (dbservice.dbTags().getFileInfoByFilePath(filePath, fileInfo)) {
        init(fileInfo);
    } else {
        qDebug() << "未能从数据库获取文件信息";
        // 即使获取失败，也初始化基本信息
        init(fileInfo);
    }
}

TagDetail::~TagDetail()
{
    delete ui;
}

void TagDetail::on_choosePathBtn_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), ".", tr("所有文件 (*.*)"));
    if (!filePath.isEmpty()) {
        // 更新表格中的文件路径
        ui->tableWidget->setItem(0, 1, new QTableWidgetItem(filePath));
        // 更新fileInfo中的路径
        fileInfo.filePath = filePath;
    }
}

void TagDetail::on_YesBtn_clicked() {
    // 获取修改后的文件路径
    QString newFilePath = ui->tableWidget->item(0, 1)->text();
    if (newFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("文件路径不能为空！"));
        return;
    }

    // 检查文件是否存在
    QFileInfo fileInfoCheck(newFilePath);
    if (!fileInfoCheck.exists()) {
        QMessageBox::warning(this, tr("警告"), tr("文件不存在！"));
        return;
    }

    // 更新提醒方式
    QString newReminderType = ui->reminderTypeComboBox->currentText();
    if (newReminderType != fileInfo.reminderType) {
        fileInfo.reminderType = newReminderType;
        // 更新数据库中的提醒方式
        if (!dbservice.dbTags().updateFileInfo(fileInfo)) {
            QMessageBox::warning(this, tr("警告"), tr("提醒方式更新失败！"));
            return;
        }
    }

    // 更新数据库
    if (dbservice.dbTags().updateFilePath(oldFilePath, newFilePath)) {
        // 更新成功
        QMessageBox::information(this, tr("提示"), tr("文件路径已成功更新！"));
        // 更新UI中的文件状态
        ui->tableWidget->setItem(4, 1, new QTableWidgetItem("文件存在"));
        // 更新oldFilePath
        oldFilePath = newFilePath;
    } else {
        QMessageBox::warning(this, tr("警告"), tr("文件路径更新失败！"));
        // 恢复原来的路径
        ui->tableWidget->setItem(0, 1, new QTableWidgetItem(oldFilePath));
        fileInfo.filePath = oldFilePath;
    }
}
