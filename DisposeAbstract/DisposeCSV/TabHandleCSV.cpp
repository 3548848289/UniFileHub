#include "TabHandleCSV.h"
#include <QMenu>
#include <QSet>
#include <algorithm>

TabHandleCSV::TabHandleCSV(const QString& filePath, QWidget *parent): TabAbstract(filePath, parent)
{
    QSplitter* splitter;
    highlightLabel = new QLabel(this);
    tableWidget = new QTableWidget(this);
    tableWidget->setSortingEnabled(true);
    tableWidget->horizontalHeader()->setSortIndicatorShown(true);
    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    splitter = new QSplitter(Qt::Vertical, this);

    splitter->addWidget(tableWidget);
    splitter->setSizes({800});
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    setLayout(layout);

    connect(tableWidget, &QTableWidget::customContextMenuRequested, this, &TabHandleCSV::showContextMenu);

    connect(tableWidget, &QAbstractItemView::clicked, this, [=](const QModelIndex &index){
        int row = index.row();
        int col = index.column();

        QTableWidgetItem *item = tableWidget->item(row, col);
        if (item && item->data(Qt::UserRole + 1).toBool()) {
            return;
        }

        foucsRow = row;
        foucsCol = col;

        QString jsonString = myJson::constructJson(localIp, "chick", foucsRow, foucsCol, "");
        if (link)
            emit dataToSend(jsonString);
    });


    connect(tableWidget, &QTableWidget::itemChanged, [=](QTableWidgetItem *item){
        adjustItem(item);
        QString jsonString = myJson::constructJson(localIp, "edited",item->row(), item->column(), item->text());
        if (link)
            emit dataToSend(jsonString);
        setContentModified(true);

    });

    connect(tableWidget, &QTableWidget::itemSelectionChanged, [=](){
        QString jsonString = myJson::constructJson(localIp, "clear",foucsRow, foucsCol, "");
        if (link)
            emit dataToSend(jsonString);
    });


}

void TabHandleCSV::setContent(const QString &text)
{
    tableWidget->clearContents();
    tableWidget->setRowCount(0);

    QStringList rows = text.split('\n', Qt::SkipEmptyParts);
    if (rows.isEmpty())
        return;

    // 取出第一行作为表头
    QStringList headers = rows.first().split(',');
    int colCount = headers.size();
    int rowCount = rows.size() - 1; // 除掉表头行

    tableWidget->setColumnCount(colCount);
    tableWidget->setRowCount(rowCount);
    tableWidget->setHorizontalHeaderLabels(headers);

    // 填充数据（从第二行开始）
    for (int i = 1; i < rows.size(); ++i) {
        QStringList cols = rows[i].split(',');
        for (int j = 0; j < colCount; ++j) {
            QString value = (j < cols.size()) ? cols[j] : "";
            tableWidget->setItem(i - 1, j, new QTableWidgetItem(value));
        }
    }

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

void TabHandleCSV::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        setContent(in.readAll());
        file.close();
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件"));
    }
    setContentModified(false);

}

QString TabHandleCSV::getContent() const
{
    return toCSV();
}

void TabHandleCSV::setLinkStatus(bool status)
{
    link = status;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &interface, interfaces) {
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        foreach (const QNetworkAddressEntry &entry, entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().toString().startsWith("127.")) {
                localIp = entry.ip().toString();
                return;
            }
        }
    }
}

bool TabHandleCSV::getLinkStatus()
{
    return link;
}

void TabHandleCSV::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << getContent();
        file.close();
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法保存文件"));
    }
}

void TabHandleCSV::loadFromInternet(const QByteArray &content)
{
    setContent(QString::fromUtf8(content));
}

QString TabHandleCSV::toCSV() const
{
    QString csvText;

    int rowCount = tableWidget->rowCount();
    int colCount = tableWidget->columnCount();

    // --- 先输出表头 ---
    QStringList headers;
    for (int j = 0; j < colCount; ++j) {
        headers << tableWidget->horizontalHeaderItem(j)->text();
    }
    csvText += headers.join(',') + '\n';

    // --- 再输出数据 ---
    for (int i = 0; i < rowCount; ++i) {
        QStringList rowValues;
        for (int j = 0; j < colCount; ++j) {
            QTableWidgetItem *item = tableWidget->item(i, j);
            rowValues << (item ? item->text() : "");
        }
        csvText += rowValues.join(',') + '\n';
    }

    return csvText;
}



void TabHandleCSV::adjustItem(QTableWidgetItem *item)
{
    tableWidget->blockSignals(true);
    item->setData(Qt::UserRole, "127.0.0.1");
    tableWidget->blockSignals(false);
    QVariant userData = item->data(Qt::UserRole);

    if (userData.isValid()) {
        QString userString = userData.toString();
//        qDebug() << "成功获取到的用户数据：" << userString;
    } else {
        qDebug() << "数据无效，可能是因为没有设置过或其他原因";
    }
}

void TabHandleCSV::addRow(bool insertAbove)
{
    int currentRow = tableWidget->currentRow();
    int insertPosition;

    if (currentRow == -1) {
        insertPosition = tableWidget->rowCount();
    } else {
        insertPosition = insertAbove ? currentRow : currentRow + 1;
    }

    tableWidget->insertRow(insertPosition);
    setContentModified(true);
}


void TabHandleCSV::addColumn(bool insertLeft)
{
    bool ok;
    QString defaultName = tr("Column %1").arg(tableWidget->columnCount() + 1);
    QString columnName = QInputDialog::getText(this, tr("New Column"), tr("Enter column name:"), QLineEdit::Normal, defaultName, &ok);

    if (ok && !columnName.isEmpty()) {
        int currentColumn = tableWidget->currentColumn();
        int insertPosition;

        if (currentColumn == -1) {
            insertPosition = tableWidget->columnCount();
        } else {
            insertPosition = insertLeft ? currentColumn : currentColumn + 1;
        }

        tableWidget->insertColumn(insertPosition);
        tableWidget->setHorizontalHeaderItem(insertPosition, new QTableWidgetItem(columnName));
        setContentModified(true);
    }
}

void TabHandleCSV::deleteSelectedRows()
{
    QModelIndexList selectedIndexes = tableWidget->selectionModel()->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请选择要删除的行或单元格"));
        return;
    }

    QSet<int> rowsToDelete;
    for (const QModelIndex &index : selectedIndexes) {
        rowsToDelete.insert(index.row());
    }

    int count = rowsToDelete.size();
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认删除"),
        tr("确定要删除选中的 %1 行吗？").arg(count),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QList<int> sortedRows = rowsToDelete.values();
        std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

        for (int row : sortedRows) {
            tableWidget->removeRow(row);
        }
        setContentModified(true);
    }
}

void TabHandleCSV::deleteSelectedColumns()
{
    QModelIndexList selectedIndexes = tableWidget->selectionModel()->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请选择要删除的列或单元格"));
        return;
    }

    QSet<int> columnsToDelete;
    for (const QModelIndex &index : selectedIndexes) {
        columnsToDelete.insert(index.column());
    }

    int count = columnsToDelete.size();
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认删除"),
        tr("确定要删除选中的 %1 列吗？").arg(count),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QList<int> sortedColumns = columnsToDelete.values();
        std::sort(sortedColumns.begin(), sortedColumns.end(), std::greater<int>());

        for (int column : sortedColumns) {
            tableWidget->removeColumn(column);
        }
        setContentModified(true);
    }
}

void TabHandleCSV::showContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);

    QAction *insertRowAbove = contextMenu.addAction(tr("在上方插入行"));
    QAction *insertRowBelow = contextMenu.addAction(tr("在下方插入行"));
    contextMenu.addSeparator();
    QAction *insertColumnLeft = contextMenu.addAction(tr("在左侧插入列"));
    QAction *insertColumnRight = contextMenu.addAction(tr("在右侧插入列"));
    contextMenu.addSeparator();
    QAction *deleteRows = contextMenu.addAction(tr("删除选中行"));
    QAction *deleteColumns = contextMenu.addAction(tr("删除选中列"));

    QAction *selectedAction = contextMenu.exec(tableWidget->viewport()->mapToGlobal(pos));

    if (selectedAction == insertRowAbove) {
        addRow(true);
    } else if (selectedAction == insertRowBelow) {
        addRow(false);
    } else if (selectedAction == insertColumnLeft) {
        addColumn(true);
    } else if (selectedAction == insertColumnRight) {
        addColumn(false);
    } else if (selectedAction == deleteRows) {
        deleteSelectedRows();
    } else if (selectedAction == deleteColumns) {
        deleteSelectedColumns();
    }
}


void TabHandleCSV::ReadfromServer(const QJsonObject& jsonObj)
{
    if (!jsonObj.contains("object") || !jsonObj["object"].isString()) {
        qDebug() << "Invalid JSON format for read operation.";
        return;
    }

    QString csvData = jsonObj["object"].toString();
    tableWidget->clear();

    QStringList lines = csvData.split("\n", Qt::SkipEmptyParts);
    if (lines.isEmpty())
        return;

    tableWidget->blockSignals(true);

    // 设置表头
    QStringList headers = lines.first().trimmed().split(",", Qt::SkipEmptyParts);
    int columnCount = headers.size();
    tableWidget->setColumnCount(columnCount);
    tableWidget->setHorizontalHeaderLabels(headers);

    // 解析CSV数据，忽略表头
    QString csvText = lines.mid(1).join("\n");
    parseCSV(csvText);
    tableWidget->blockSignals(false);

}

void TabHandleCSV::parseCSV(const QString &csvText)
{
    tableWidget->clear();
    QStringList rows = csvText.split('\n');
    tableWidget->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i)
    {
        QStringList cols = rows[i].split(',');
        tableWidget->setColumnCount(qMax(tableWidget->columnCount(), cols.size()));
        for (int j = 0; j < cols.size(); ++j)
            tableWidget->setItem(i, j, new QTableWidgetItem(cols[j]));
    }
}

void TabHandleCSV::ChickfromServer(const QJsonObject& jsonObj)
{
    auto [ip, row, col, newValue] = myJson::extract_common_fields(jsonObj);
    if (row >= 0 && row < tableWidget->rowCount() && col >= 0 && col < tableWidget->columnCount()) {
        QTableWidgetItem *item = tableWidget->item(row, col);
        if (item) {
            tableWidget->blockSignals(true);
            item->setBackground(QColor(0, 120, 215)); // 远程编辑颜色
            item->setData(Qt::UserRole, ip.value_or("unknown"));  // Store IP
            item->setData(Qt::UserRole + 1, true);  // 设置正在被远程编辑的标志
            item->setToolTip(QString("正在被 %1 编辑中").arg(ip.value_or("unknown")));
            // tableWidget->clearSelection();
            tableWidget->blockSignals(false);
        }
    } else {
        qDebug() << "Invalid row or column index: (" << row << ", " << col << ")";
    }
}

void TabHandleCSV::clearfromServer(const QJsonObject& jsonObj)
{
    auto [ip, row, col, newValue] = myJson::extract_common_fields(jsonObj);
    if (row >= 0 && row < tableWidget->rowCount() && col >= 0 && col < tableWidget->columnCount()) {
        QTableWidgetItem *item = tableWidget->item(row, col);
        if (item) {
            tableWidget->blockSignals(true);
            item->setBackground(Qt::transparent); // 或使用白色 QColor(Qt::white)
            item->setData(Qt::UserRole + 1, false);  // 取消远程编辑标志
            item->setToolTip("");
            tableWidget->blockSignals(false);
        }
    } else {
        qDebug() << "Invalid row or column index: (" << row << ", " << col << ")";
    }
}


void TabHandleCSV::editedfromServer(const QJsonObject& jsonObj)
{
    auto [ip, row, col, newValue] = myJson::extract_common_fields(jsonObj);
    if (row >= 0 && row < tableWidget->rowCount() && col >= 0 && col < tableWidget->columnCount()) {
        QTableWidgetItem *item = tableWidget->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            tableWidget->setItem(row, col, item);
        }
        item->setText(newValue.value());
        adjustItem(item);
    } else {
        qDebug() << "Invalid row or column index: (" << row << ", " << col << ")";
    }
}


void TabHandleCSV::findNext(const QString &str, Qt::CaseSensitivity cs)
{
    bool found = false;
    int rowCount = tableWidget->rowCount();
    int colCount = tableWidget->columnCount();

    QTableWidgetItem *currentItem = tableWidget->currentItem();
    if (!currentItem) return;

    int currentRow = currentItem->row();
    int currentCol = currentItem->column();

    for (int row = currentRow; row < rowCount; ++row) {
        for (int col = (row == currentRow ? currentCol + 1 : 0); col < colCount; ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item && item->text().contains(str, cs)) {
                tableWidget->setCurrentItem(item);
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        for (int row = 0; row <= currentRow; ++row) {
            for (int col = 0; col < (row == currentRow ? currentCol : colCount); ++col) {
                QTableWidgetItem *item = tableWidget->item(row, col);
                if (item && item->text().contains(str, cs)) {
                    tableWidget->setCurrentItem(item);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    if (!found) {
        QMessageBox::information(this, tr("查找"), tr("找不到此单词"));
    }
}

void TabHandleCSV::findAll(const QString &str, Qt::CaseSensitivity cs)
{
    bool found = false;
    int rowCount = tableWidget->rowCount();
    int colCount = tableWidget->columnCount();

    tableWidget->blockSignals(true);

    // 遍历所有单元格，查找匹配项
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item && item->text().contains(str, cs == Qt::CaseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive)) {
                // 找到匹配项，设置高亮
                item->setForeground(QBrush(Qt::red));
                found = true;
            }
        }
    }

    tableWidget->blockSignals(false);

    if (!found)
        QMessageBox::information(this, tr("查找"), tr("找不到此单词"));

}

void TabHandleCSV::clearHighlight()
{
    int rowCount = tableWidget->rowCount();
    int colCount = tableWidget->columnCount();

    tableWidget->blockSignals(true);

    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item) {
                item->setForeground(QBrush());  // 使用默认字体色
            }
        }
    }

    tableWidget->blockSignals(false);
}


