#include "MoreFunction.h"
#include "ui_MoreFunction.h"
#include <QDate>
#include <QtGlobal>

MoreFunction::MoreFunction(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MoreFunction)
{
    ui->setupUi(this);

    // 版本标识按编译条件区分平台/编译架构；末段为日期版本号（月日，如 1011）
    const QString dateVersion = QDate::currentDate().toString(QStringLiteral("MMdd"));
#ifdef Q_OS_WIN
    ui->label_4->setText(QStringLiteral("UniFileHub for Windows 1.0.") + dateVersion);
#elif defined(Q_OS_LINUX)
#if QT_VERSION_MAJOR >= 6
    ui->label_4->setText(QStringLiteral("UniFileHub for Linux Qt6 x86_64 1.0.") + dateVersion);
#elif defined(__aarch64__) || defined(_M_ARM64)
    ui->label_4->setText(QStringLiteral("UniFileHub for Linux arm_64 1.0.") + dateVersion);
#else
    ui->label_4->setText(QStringLiteral("UniFileHub for Linux Qt5 x86_64 1.0.") + dateVersion);
#endif
#endif
}

MoreFunction::~MoreFunction()
{
    delete ui;
}
