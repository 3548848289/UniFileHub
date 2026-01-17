#include "./include/WidgetFunctional.h"
#include "ui/ui_WidgetFunctional.h"
#include "../../ClipBoard/include/ClipboardComponentFactory.h"
#include "../../Setting/include/ThemeManager.h"

WidgetFunctional::~WidgetFunctional()
{
    delete ui;
}

DInfo* WidgetFunctional::getDInfo() {
    return dinfo;
}

void WidgetFunctional::on_pushButton_1_clicked() {
    emit showFiletag();
}

void WidgetFunctional::on_pushButton_2_clicked()
{
    emit showFilebackup();
}


void WidgetFunctional::on_pushButton_3_clicked() {
    emit showWSchedule();

}


void WidgetFunctional::on_pushButton_4_clicked()
{
    emit showwOnlinedoc();

}

void WidgetFunctional::on_pushButton_5_clicked()
{
    emit showDraw();
}

void WidgetFunctional::on_pushButton_6_clicked()
{
    emit showDrive(drive);
}


void WidgetFunctional::on_pushButton_7_clicked()
{
    emit sendEmailForm(form);
}

void WidgetFunctional::on_pushButton_8_clicked()
{
    emit showClipboard(clipboard);

}


void WidgetFunctional::on_pushButton_9_clicked()
{

    dlogin->exec();
}

void WidgetFunctional::on_pushButton_10_clicked()
{
    // 检查是否已经存在MoreFunction窗口，如果存在则显示它，不存在才创建新窗口
    if (!more_function) {
        more_function = new MoreFunction();
        // 设置窗口属性，确保在关闭时删除对象并重置指针
        connect(more_function, &QWidget::destroyed, this, [this]() {
            more_function = nullptr;
        });
    }
    // 显示窗口并将其置于前台
    more_function->show();
    more_function->raise();
    more_function->activateWindow();
}



void WidgetFunctional::handleLoginSuccess(const QString& username) {
    qDebug() << "Username in handleLoginSuccess:" << username;
    dinfo = new DInfo(username, this);
}

WidgetFunctional::WidgetFunctional(QWidget *parent)
    : QWidget(parent), ui(new Ui::WidgetFunctional), more_function(nullptr)
{
    ui->setupUi(this);
    btnGroup=new QButtonGroup;
    for (int i = 1; i <= 10; ++i) {
        QPushButton *button = findChild<QPushButton*>(QString("pushButton_%1").arg(i));        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        if (button) {
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            btnGroup->addButton(button);
            button->setCheckable(true);

            connect(button, &QPushButton::clicked, this, [=]() {
                foreach(QAbstractButton *btn, btnGroup->buttons()) {
                    btn->setChecked(btn->objectName() == QString("pushButton_%1").arg(i));
                }
            });
        }
    }

   // 使用ThemeManager统一管理样式
    QString primaryColor = ThemeManager::Instance().primaryColor().name();
    QString hoverColor = primaryColor;

    QString styleSheet = QString(
        "QPushButton#pushButton_1:checked, QPushButton#pushButton_2:checked,"
        "QPushButton#pushButton_3:checked, QPushButton#pushButton_4:checked,"
        "QPushButton#pushButton_5:checked, QPushButton#pushButton_6:checked,"
        "QPushButton#pushButton_7:checked, QPushButton#pushButton_8:checked,"
        "QPushButton#pushButton_9:checked, QPushButton#pushButton_10:checked"
        "{background:transparent;border:none; border-bottom:3px solid %1;color:%1;}"

        "QPushButton#pushButton_1:hover, QPushButton#pushButton_2:hover,"
        "QPushButton#pushButton_3:hover, QPushButton#pushButton_4:hover,"
        "QPushButton#pushButton_5:hover, QPushButton#pushButton_6:hover,"
        "QPushButton#pushButton_7:hover, QPushButton#pushButton_8:hover,"
        "QPushButton#pushButton_9:hover, QPushButton#pushButton_10:hover"
        "{background:transparent;border:none; border-bottom:3px solid %1;color:%1;}"
    ).arg(primaryColor);

    this->setStyleSheet(styleSheet);


    // 设置所有按钮的自定义图标
    ui->pushButton_1->setIcon(IconManager::icon(IconManager::Icon::FileSystem, QSize(24,24)));
    ui->pushButton_2->setIcon(IconManager::icon(IconManager::Icon::FileBackup, QSize(24,24)));
    ui->pushButton_3->setIcon(IconManager::icon(IconManager::Icon::Tag, QSize(24,24)));
    ui->pushButton_4->setIcon(IconManager::icon(IconManager::Icon::OnlineDoc, QSize(24,24)));
    ui->pushButton_5->setIcon(IconManager::icon(IconManager::Icon::Drawing, QSize(24,24)));
    ui->pushButton_6->setIcon(IconManager::icon(IconManager::Icon::Drive, QSize(24,24)));
    ui->pushButton_7->setIcon(IconManager::icon(IconManager::Icon::Email, QSize(24,24)));
    ui->pushButton_8->setIcon(IconManager::icon(IconManager::Icon::Clipboard, QSize(24,24)));
    ui->pushButton_9->setIcon(IconManager::icon(IconManager::Icon::Login, QSize(24,24)));
    ui->pushButton_10->setIcon(IconManager::icon(IconManager::Icon::More, QSize(24,24)));

    // ui->pushButton_7->hide(); //暂时不隐藏
    form = new SendEmail();
    clipboard = ClipboardComponentFactory::createClipboardComponent();
    dlogin = new DLogin();
    drive = new DriveView();

    connect(dlogin, &DLogin::loginSuccessful, this, &WidgetFunctional::handleLoginSuccess);

}


void WidgetFunctional::toggleButtonVisibility(int buttonIndex)
{
    QPushButton *button = nullptr;
    switch (buttonIndex) {
    case 1: button = ui->pushButton_1; break;
    case 2: button = ui->pushButton_2; break;
    case 3: button = ui->pushButton_3; break;
    case 4: button = ui->pushButton_4; break;
    case 5: button = ui->pushButton_5; break;
    case 6: button = ui->pushButton_6; break;
    case 7: button = ui->pushButton_7; break;
    case 8: button = ui->pushButton_8; break;
    case 9: button = ui->pushButton_9; break;
    case 10: button = ui->pushButton_10; break;
    default: return;
    }

    if (button) {
        bool isVisible = button->isVisible();
        button->setVisible(!isVisible);
        emit buttonVisibilityChanged(buttonIndex, !isVisible);
    }
}
