#include "include/DLogin.h"
#include "ui/ui_DLogin.h"
#include <QMessageBox>
#include <QFileDialog>
#include "../Setting/include/SettingManager.h"
#include "../Setting/include/ThemeManager.h"
#include "../Setting/include/IconManager.h"

DLogin::DLogin(QWidget *parent): QDialog(parent), ui(new Ui::DLogin)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    ui->loginBtn->setEnabled(false);
    ui->registerBtn->setEnabled(false);
    
    // 设置关闭按钮图标
    ui->exit_toolButton->setIcon(IconManager::icon(IconManager::Icon::Close, QSize(12, 12)));

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

    // 连接隐私政策和忘记密码按钮的点击事件
    connect(ui->resiger_pushButton, &QPushButton::clicked, this, &DLogin::onPrivacyPolicyClicked);
    connect(ui->forget_pushButton, &QPushButton::clicked, this, &DLogin::onForgotPasswordClicked);


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
        QMessageBox::warning(this, "提示", "头像加载失败，请重试");
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
        QMessageBox::warning(this, "提示", "请输入用户名和密码");
        return;
    }

    flaskinfo->route_loginUser(usernameText, passwordText);
}

void DLogin::on_registerBtn_clicked()
{
    QString usernameText = ui->username->text();
    QString passwordText = ui->password->text();

    if (avatarImage.isNull()) {
        QMessageBox::warning(this, "提示", "请先上传头像");
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
        QMessageBox::information(this, "登录成功", "登录成功！欢迎回来");
        this->close();
    }
    else {
        // 优化登录失败提示语
        QString errorMsg = message;
        if (message.contains("password")) {
            errorMsg = "账户或密码错误，请重新输入";
        } else if (message.contains("user")) {
            errorMsg = "用户不存在，请先注册";
        }
        QMessageBox::warning(this, "登录失败", errorMsg);
    }
}

void DLogin::onRegisterResponse(const QJsonObject &response)
{
    qDebug() << response;

    QString message = response.value("message").toString();
    QString error   = response.value("error").toString();

    if (message == "User registered successfully!") {
        QMessageBox::information(this, "注册成功", "注册成功！您现在可以登录了");
        this->close();
        return;
    }

    // 优化注册失败提示
    QString errorMsg = !error.isEmpty() ? error : message;

    if (errorMsg.contains("Username") && errorMsg.contains("exists")) {
        errorMsg = "用户名已存在，请选择其他用户名";
    } else if (errorMsg.contains("password")) {
        errorMsg = "密码格式不正确，请检查密码设置";
    }

    QMessageBox::critical(this, "注册失败", errorMsg);
}

void DLogin::onNetworkError(const QString &error)
{
    // 优化网络错误提示语
    QString errorMsg = "网络连接失败，请检查您的网络设置后重试";
    if (error.contains("timeout")) {
        errorMsg = "网络连接超时，请稍后重试";
    } else if (error.contains("host not found")) {
        errorMsg = "无法连接到服务器，请稍后重试";
    }
    QMessageBox::critical(this, "网络错误", errorMsg);
}

void DLogin::onPrivacyPolicyClicked()
{
    QMessageBox msg(nullptr);
    msg.setWindowTitle("隐私政策");
    msg.setText(
        "感谢您使用我们的产品！\n\n"
        "我们承诺保护您的隐私和个人信息安全。\n\n"
        "1. 我们仅收集必要的用户信息用于身份验证。\n"
        "2. 您的个人信息将严格保密，不会泄露给第三方。\n"
        "3. 您有权随时查看、修改或删除您的个人信息。\n\n"
        "如有任何疑问，请联系我们的客服团队。"
        );

    msg.setIcon(QMessageBox::NoIcon);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.exec();
}

void DLogin::onForgotPasswordClicked()
{
    QMessageBox msg(nullptr);
    msg.setWindowTitle("忘记密码");
    msg.setText(
        "如果您忘记了密码，请联系作者邮箱：\n"
        "Layeep@outlook.com\n\n"
        "我们将尽快为您处理密码重置请求。"
        );

    msg.setIcon(QMessageBox::NoIcon);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.exec();
}
