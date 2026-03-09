#include "TabHandleXLSX.h"
#include "xlsxdocument.h"
#include "xlsxworkbook.h"
#include "xlsxworksheet.h"
#include <QComboBox>
#include <QSignalBlocker>

TabHandleXLSX::TabHandleXLSX(const QString& filePath, QWidget *parent) : TabAbstract(filePath, parent) {
    tableWidget = new QTableWidget(this);
    tableWidget->setSortingEnabled(true);
    tableWidget->horizontalHeader()->setSortIndicatorShown(true);
    tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // 添加sheet切换下拉框
    sheetComboBox = new QComboBox(this);
    connect(sheetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TabHandleXLSX::onSheetChanged);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(tableWidget);
    splitter->setSizes({700, 100});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(sheetComboBox);
    layout->addWidget(splitter);
    setLayout(layout);

    connect(tableWidget, &QTableWidget::itemChanged, this, [this]() {
        setContentModified(true);
    });


}

void TabHandleXLSX::setContent(const QString &text) {
    Q_UNUSED(text);
    QMessageBox::information(this, tr("不支持"), tr("XLSX 文件不支持直接文本输入。"));
}

QString TabHandleXLSX::getContent() const {
    return tr("[XLSX table view]");
}

void TabHandleXLSX::loadFromFile(const QString &fileName) {
    QXlsx::Document xlsx(fileName);
    if (!QFile::exists(fileName) || !xlsx.load()) {
        QMessageBox::warning(this, tr("错误"), tr("无法加载 Excel 文件。"));
        return;
    }

    // 保存文件路径和Document引用，用于切换sheet时重新加载数据
    currentFilePath = fileName;
    
    // 获取工作簿中的所有sheet名称
    QXlsx::Workbook *workbook = xlsx.workbook();
    if (!workbook) {
        QMessageBox::warning(this, tr("错误"), tr("无法获取工作簿。"));
        return;
    }
    
    // 清空下拉框并添加所有sheet名称
    sheetComboBox->clear();
    for (int i = 0; i < workbook->sheetCount(); ++i) {
        QXlsx::AbstractSheet *sheet = workbook->sheet(i);
        if (sheet) {
            sheetComboBox->addItem(sheet->sheetName(), i);
        }
    }
    
    // 加载第一个sheet的数据
    if (sheetComboBox->count() > 0) {
        loadSheetData(0);
    }
    
    setContentModified(false);
}

void TabHandleXLSX::loadSheetData(int sheetIndex) {
    QXlsx::Document xlsx(currentFilePath);
    if (!xlsx.load()) {
        QMessageBox::warning(this, tr("错误"), tr("无法重新加载 Excel 文件。"));
        return;
    }

    // 切换/加载sheet时避免触发itemChanged导致误判为已修改
    QSignalBlocker blocker(tableWidget);
    
    // 获取指定索引的sheet
    QXlsx::Workbook *workbook = xlsx.workbook();
    QXlsx::AbstractSheet *abstractSheet = workbook->sheet(sheetIndex);
    if (!abstractSheet || abstractSheet->sheetType() != QXlsx::AbstractSheet::ST_WorkSheet) {
        QMessageBox::warning(this, tr("错误"), tr("无效的工作表。"));
        return;
    }
    
    // 设置活动sheet
    workbook->setActiveSheet(sheetIndex);
    
    // 计算最大行列数
    int maxRow = 0, maxCol = 0;
    for (int row = 1; row <= 1000; ++row) {
        for (int col = 1; col <= 100; ++col) {
            if (xlsx.cellAt(row, col)) {
                maxRow = qMax(maxRow, row);
                maxCol = qMax(maxCol, col);
            }
        }
    }
    
    // 清空表格并设置行列数
    tableWidget->clearContents();
    tableWidget->setRowCount(maxRow);
    tableWidget->setColumnCount(maxCol);
    
    // 读取数据并填充表格
    for (int row = 1; row <= maxRow; ++row) {
        for (int col = 1; col <= maxCol; ++col) {
            QVariant value = xlsx.read(row, col);
            if (!value.isNull()) {
                tableWidget->setItem(row - 1, col - 1, new QTableWidgetItem(value.toString()));
            }
        }
    }

    tableWidget->resizeColumnsToContents();

    // 填充完表格数据后
    tableWidget->resizeColumnsToContents(); // 先自适应内容
    for (int col = 0; col < tableWidget->columnCount(); ++col) {
        int w = tableWidget->columnWidth(col);
        if (w > 300)
            tableWidget->setColumnWidth(col, 300);
        else if (w < 50)
            tableWidget->setColumnWidth(col, 50);
    }

}

void TabHandleXLSX::onSheetChanged(int index) {
    if (index >= 0 && !currentFilePath.isEmpty()) {
        // 获取sheet索引并加载数据
        int sheetIndex = sheetComboBox->itemData(index).toInt();
        loadSheetData(sheetIndex);
    }
}

void TabHandleXLSX::saveToFile(const QString &fileName){
    // 读取已有文件以保留其它sheet
    QXlsx::Document xlsx(fileName);
    if (QFile::exists(fileName) && !xlsx.load()) {
        QMessageBox::warning(this, tr("保存错误"), tr("无法加载现有 XLSX 文件，已取消保存以避免丢失工作表。"));
        return;
    }

    // 获取当前sheet名称
    QString currentSheetName = sheetComboBox->currentText();

    // 确保当前sheet存在并选中
    QStringList sheetNames = xlsx.sheetNames();
    if (!sheetNames.contains(currentSheetName)) {
        if (sheetNames.size() == 1) {
            xlsx.renameSheet(sheetNames.first(), currentSheetName);
        } else {
            xlsx.addSheet(currentSheetName);
        }
    }
    xlsx.selectSheet(currentSheetName);

    // 写入当前表格数据到当前sheet
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item && !item->text().isEmpty()) {
                xlsx.write(row + 1, col + 1, item->text());
            }
        }
    }
    
    if (!xlsx.saveAs(fileName)) {
        QMessageBox::warning(this, tr("保存错误"), tr("保存 XLSX 文件失败。"));
    } else {
        setContentModified(false);
    }
}

void TabHandleXLSX::loadFromInternet(const QByteArray &content) {
    QString tempFilePath = QDir::temp().filePath("temp_internet.xlsx");
    QFile tempFile(tempFilePath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(content);
        tempFile.close();
        loadFromFile(tempFilePath);
        tempFile.remove();
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法写入临时文件。"));
    }
}

void TabHandleXLSX::ControlWidget(bool judge) {
    qDebug() << "TabHandleXLSX: Control panel slot called";
}
