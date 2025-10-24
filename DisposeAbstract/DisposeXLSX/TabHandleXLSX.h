#ifndef TABHANDLEXLSX_H
#define TABHANDLEXLSX_H
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>
#include <QDir>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QString>
#include <QWidget>
#include <QFile>
#include <QComboBox>
#include <QHeaderView>
#include "../../main/include/TabAbstract.h"

class TabHandleXLSX : public TabAbstract
{
    Q_OBJECT
public:

    virtual void setContent(const QString &text) override;
    virtual QString getContent() const override;
    void loadFromFile(const QString &fileName) override;
    void loadFromInternet(const QByteArray &content) override;
    void saveToFile(const QString &fileName) override;
    void ControlWidget(bool judge) override;


    TabHandleXLSX(const QString& filePath,QWidget *parent = nullptr);

private slots:
    void onSheetChanged(int index);

private:
    QTableWidget *tableWidget;
    QComboBox *sheetComboBox; // 用于切换sheet的下拉框
    QString currentFilePath; // 当前文件路径

    // 加载指定sheet的数据
    void loadSheetData(int sheetIndex);

};

#endif // TABHANDLEXLSX_H
