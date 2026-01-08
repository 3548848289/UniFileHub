#include "ControlWidTXT.h"
#include "ui/ui_ControlWidTXT.h"

ControlWidTXT::ControlWidTXT(QWidget *parent) : QWidget(parent), ui(new Ui::ControlWidTXT), m_currentCodecName("UTF-8")
{
    ui->setupUi(this);
    bool isShow = SettingManager::Instance().file_see_txt();
    this->setVisible(isShow);
    
    // 获取所有可用的编码名称
    QList<QByteArray> codecNames = {"UTF-8", "GBK", "UTF-16", "UTF-16LE", "UTF-16BE"};
    
    // 填充编码下拉框
    for (const QByteArray& codecName : codecNames) {
        QString name = QString::fromLatin1(codecName);
        ui->encodingComboBox->addItem(name, QVariant(codecName));
    }
    
    // 设置默认编码为UTF-8
    setCurrentCodecName("UTF-8");
    
    // 连接编码选择变化信号
    connect(ui->encodingComboBox, &QComboBox::currentTextChanged, this, &ControlWidTXT::onEncodingComboBoxCurrentIndexChanged);
}

ControlWidTXT::~ControlWidTXT()
{
    delete ui;
}



QString ControlWidTXT::getCurrentCodecName() const
{
    return m_currentCodecName;
}

void ControlWidTXT::setCurrentCodecName(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        int index = ui->encodingComboBox->findText(codecName);
        if (index != -1) {
            ui->encodingComboBox->setCurrentIndex(index);
        }
    }
}

void ControlWidTXT::onEncodingComboBoxCurrentIndexChanged(const QString& codecName)
{
    if (m_currentCodecName != codecName) {
        m_currentCodecName = codecName;
        emit encodingChanged(codecName);
    }
}
