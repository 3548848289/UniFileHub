#include "include/DLogin.h"
#include "ui/ui_DLogin.h"
#include <QMessageBox>
#include <QFileDialog>
#include "../Setting/include/SettingManager.h"
#include "../Setting/include/ThemeManager.h"

DLogin::DLogin(QWidget *parent): QDialog(parent), ui(new Ui::DLogin)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    ui->loginBtn->setEnabled(false);
    ui->registerBtn->setEnabled(false);

    // 使用ThemeManager设置样式
    QString secondaryColor = ThemeManager::Instance().secondaryColor().name();
    QString lightSecondaryColor = ThemeManager::Instance().secondaryColor().lighter(150).name();
    
    // 设置头像按钮样式
    ui->avatar_pushButton->setStyleSheet(QString(
        "QPushButton {"
        "    color: %1;"
        "    background-color: %2;  "
        "    border: 1px solid %1; "
        "    border-radius: 25px; "
        "    padding: 10px;  "
        "    width: 50px; "
        "    height: 50px;"
        "    text-align: center; "
        "}"
    ).arg(secondaryColor, lightSecondaryColor));
    
    // 初始化登录和注册按钮的初始样式
    ui->loginBtn->setStyleSheet(QString("background-color: %1; color: rgb(255, 255, 255); border: none; border-radius: 10px;").arg(lightSecondaryColor));
    ui->registerBtn->setStyleSheet(QString("background-color: %1; color: rgb(255, 255, 255); border: none; border-radius: 10px;").arg(lightSecondaryColor));

    flaskinfo = new FlaskInfo(this);

    connect(flaskinfo, &FlaskInfo::s_loginRec, this, &DLogin::onLoginResponse);
    connect(flaskinfo, &FlaskInfo::s_registerRec, this, &DLogin::onRegisterResponse);
    connect(flaskinfo, &FlaskInfo::errorOccurred, this, &DLogin::onNetworkError);
    connect(flaskinfo, &FlaskInfo::avatarDownloaded, this, &DLogin::onAvatarDownloaded);


}


void DLogin::onAvatarDownloaded(const QByteArray &data, const QString &action)
{
    QImage image;
    if (image.loadFromData(data)) {
        m_avatarImage = image; // 保存图片到成员变量
        if (action == "login_avatar") {
            ui->avatar_pushButton->setIcon(QIcon(QPixmap::fromImage(m_avatarImage)));  // 使用成员变量
        } else if (action == "load_user_avatar") {
            qDebug() << "User avatar loaded for action:" << action;
        }
    } else {
        QMessageBox::warning(this, "错误", "头像加载失败");
    }
}


void DLogin::on_radioButton_clicked() {
    QString secondaryColor = ThemeManager::Instance().secondaryColor().name();
    QString lightSecondaryColor = ThemeManager::Instance().secondaryColor().lighter(150).name();
    
    if (ui->radioButton->isChecked()) {
        ui->loginBtn->setStyleSheet(QString("background-color: %1;").arg(secondaryColor));
        ui->loginBtn->setEnabled(true);
        ui->registerBtn->setStyleSheet(QString("background-color: %1;").arg(secondaryColor));
        ui->registerBtn->setEnabled(true);
    } else {
        ui->loginBtn->setStyleSheet(QString("background-color: %1;").arg(lightSecondaryColor));
        ui->loginBtn->setEnabled(false);
        ui->registerBtn->setStyleSheet(QString("background-color: %1;").arg(lightSecondaryColor));
        ui->registerBtn->setEnabled(false);
    }
}

void DLogin::on_exit_toolButton_clicked() {
    ui->username->clear();
    ui->password->clear();
    ui->avatar_pushButton->setIcon(QIcon());
    ui->statusLabel->clear();
    close();
}

void DLogin::on_avatar_pushButton_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "选择头像", "", "Images (*.png *.jpg *.bmp)");
    if (!fileName.isEmpty()) {
        QPixmap avatar(fileName);
        avatarImage = avatar;
        if (!avatar.isNull()) {
            ui->avatar_pushButton->setIcon(QIcon(avatar));
            ui->avatar_pushButton->setIconSize(ui->avatar_pushButton->size());
        } else {
            qWarning("无法加载图像: %s", qPrintable(fileName));
        }
    }
}

DLogin::~DLogin() {
    delete ui;
    delete flaskinfo;
}

void DLogin::on_loginBtn_clicked()
{
    QString usernameText = ui->username->text();
    QString passwordText = ui->password->text();

    if (usernameText.isEmpty() || passwordText.isEmpty()) {
        QMessageBox::warning(this, "警告", "用户名和密码不能为空");
        return;
    }

    flaskinfo->route_loginUser(usernameText, passwordText);
}

void DLogin::on_registerBtn_clicked()
{
    QString usernameText = ui->username->text();
    QString passwordText = ui->password->text();

    if (avatarImage.isNull()) {
        QMessageBox::warning(this, "警告", "请先上传头像");
        return;
    }

    QByteArray avatarData;
    QBuffer buffer(&avatarData);
    avatarImage.save(&buffer, "PNG");

    flaskinfo->route_registerUser(usernameText, passwordText, avatarData);
}

void DLogin::onLoginResponse(const QJsonObject &response)
{

    QString message = response["message"].toString();

    if (message == "Login successful") {
        if (!m_avatarImage.isNull())
            ui->avatar_pushButton->setIcon(QIcon(QPixmap::fromImage(m_avatarImage)));  // 直接使用保存的头像
        else
            qDebug() << "Avatar image not available.";

        // 从登录响应中获取token并存储
        if (response.contains("access_token")) {
            m_token = response["access_token"].toString();
            SettingManager::Instance().setToken(m_token);
        }
        
        emit loginSuccessful(response["username"].toString());
        QMessageBox::information(this, "登录成功", message);
        this->close();
    }
    else
        QMessageBox::warning(this, "登录失败", message);
}

void DLogin::onRegisterResponse(const QJsonObject &response)
{
    QString message = response["message"].toString();
    if (message == "User registered successfully!") {
        QMessageBox::information(this, "注册成功", message);
        this->close();

    } else {
        QMessageBox::critical(this, "注册失败", message);
    }
}

void DLogin::onNetworkError(const QString &error)
{
    QMessageBox::warning(this, "网络错误", error);
}
