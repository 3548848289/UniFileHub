#include "SendEmail.h"
#include "ui_SendEmail.h"

#include "server.h"
#include "serverreply.h"

#include <QDebug>
#include <QErrorMessage>
#include <QFileDialog>
#include <QMessageBox>
#include <QSslError>
#include "SimpleMail.h"

using namespace SimpleMail;

SendEmail::SendEmail(QWidget *parent): QWidget(parent), ui(new Ui::SendEmail)
{
    ui->setupUi(this);
    ui->attachments->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 启用拖拽功能
    ui->attachments->setAcceptDrops(true);
    ui->attachments->setDropIndicatorShown(true);

    QSettings m_settings("settings.ini", QSettings::IniFormat);
    ui->host->setText(m_settings.value("EmailConfig/host", "localhost").toString());
    ui->port->setValue(m_settings.value("EmailConfig/port", 465).toInt());
    ui->username->setText(m_settings.value("EmailConfig/username").toString());
    ui->password->setText(m_settings.value("EmailConfig/password").toString());
    ui->security->setCurrentIndex(m_settings.value("EmailConfig/ssl").toInt());
    ui->sender->setText(m_settings.value("EmailConfig/sender").toString());
    ui->recipients->setText(m_settings.value("EmailConfig/received").toString());
    ui->security->setCurrentIndex(1);
    for (int i = 0; i < ui->gridLayout->count(); ++i) {
        QLayoutItem *item = ui->gridLayout->itemAt(i);
        if (item) {
            QWidget *widget = item->widget();
            if (widget)
                widget->hide();
        }
    }
}

SendEmail::~SendEmail()
{
    delete ui;
}

void SendEmail::on_addAttachment_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    if (dialog.exec()) {
        QStringList selectedFiles = dialog.selectedFiles();

        for (const QString &filePath : selectedFiles) {
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.fileName();

            QListWidgetItem* item = new QListWidgetItem(fileName);
            item->setData(Qt::UserRole, filePath);
            item->setToolTip(filePath);
            ui->attachments->addItem(item);
        }
    }
}

void SendEmail::on_sendEmail_clicked()
{
    MimeMessage message;

    message.setSender(EmailAddress{ui->sender->text()});
    message.setSubject(ui->subject->text());

    const QStringList rcptStringList =
        ui->recipients->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &to : rcptStringList) {
        message.addTo(EmailAddress{to});
    }
    message.addPart(std::make_shared<MimeHtml>(ui->texteditor->toHtml()));

    for (int i = 0; i < ui->attachments->count(); ++i) {
        QString fullFilePath = ui->attachments->item(i)->data(Qt::UserRole).toString();
        QFile *file = new QFile(fullFilePath);

        if (!file->exists() || !file->open(QIODevice::ReadOnly)) {
            delete file;
            QMessageBox::warning(this, "该文件发送出错",
                    QString("该文件可能已被删除:\n%1").arg(fullFilePath));
            continue;
        }
        message.addPart(std::make_shared<MimeAttachment>(std::shared_ptr<QFile>(file)));
    }
    sendMailAsync(message);    
}

void SendEmail::sendEmailWithData(const QString &subject, const QString &bodyHtml,
        const QStringList &attachments) {
    MimeMessage message;
    message.setSender(EmailAddress{ui->sender->text()});
    message.setSubject(subject);

    const QStringList rcptStringList =
        ui->recipients->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &to : rcptStringList) {
        message.addTo(EmailAddress{to});
    }

    message.addPart(std::make_shared<MimeHtml>(bodyHtml));
    for (const QString &filePath : attachments) {
        message.addPart(std::make_shared<MimeAttachment>(
            std::make_shared<QFile>(filePath)));
    }
    sendMailAsync(message);
}

void SendEmail::sendMailAsync(const MimeMessage &msg)
{
    const QString host = ui->host->text();
    const quint16 port(ui->port->value());
    const Server::ConnectionType ct =
        ui->security->currentIndex() == 0 ? Server::TcpConnection :
            ui->security->currentIndex() == 1 ? Server::SslConnection : Server::TlsConnection;

    // 每次都新建，不要复用
    Server *server = new Server(this);
    connect(server, &Server::sslErrors, this, [](const QList<QSslError> &errors) {
        qDebug() << "Server SSL errors" << errors.size();
    });
    server->setHost(host);
    server->setPort(port);
    server->setConnectionType(ct);

    const QString user = ui->username->text();
    if (!user.isEmpty()) {
        server->setAuthMethod(Server::AuthLogin);
        server->setUsername(user);
        server->setPassword(ui->password->text());
    }

    ServerReply *reply = server->sendMail(msg);
    connect(reply, &ServerReply::finished, this, [=] {
        qDebug() << "ServerReply finished" << reply->error() << reply->responseText();

        if (reply->error()) {
            errorMessage(QStringLiteral("Mail sending failed:\n") + reply->responseText());
        } else {
            QMessageBox::information(this, "邮件提醒", "发送成功！");
        }

        reply->deleteLater();
        server->deleteLater(); // 用完销毁
    });
}

void SendEmail::errorMessage(const QString &message)
{
    QErrorMessage err(this);
    err.showMessage(message);
    err.exec();
}

void SendEmail::on_attachments_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* item = ui->attachments->itemAt(pos);
    if (!item) return;
    QMenu contextMenu(this);
    QAction *removeAction = new QAction("移除", this);
    connect(removeAction, &QAction::triggered, this, [this, item]() {
        int row = ui->attachments->row(item);
        delete ui->attachments->takeItem(row);
    });
    contextMenu.addAction(removeAction);
    contextMenu.exec(ui->attachments->viewport()->mapToGlobal(pos));
}

void SendEmail::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void SendEmail::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl &url : urlList) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                QFileInfo fileInfo(filePath);
                QString fileName = fileInfo.fileName();

                QListWidgetItem* item = new QListWidgetItem(fileName);
                item->setData(Qt::UserRole, filePath);
                item->setToolTip(filePath);
                ui->attachments->addItem(item);
            }
        }
        event->acceptProposedAction();
    }
}

