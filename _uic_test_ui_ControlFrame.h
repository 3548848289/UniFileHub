/********************************************************************************
** Form generated from reading UI file 'ControlFrame.ui'
**
** Created by: Qt User Interface Compiler version 6.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef _UIC_TEST_UI_CONTROLFRAME_H
#define _UIC_TEST_UI_CONTROLFRAME_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ControlFrame
{
public:
    QGridLayout *gridLayout_2;
    QLabel *label_4;
    QSlider *translateSlider;
    QSlider *rotateSlider;
    QLabel *label_3;
    QLabel *label_2;
    QSlider *scaleSlider;
    QSlider *shearSlider;
    QLabel *label_1;
    QGridLayout *gridLayout;
    QPushButton *exportButton;
    QPushButton *addWatermarkButton;
    QPushButton *drawButton;
    QWidget *drawPanel;
    QHBoxLayout *horizontalLayout_draw;
    QComboBox *drawToolCombo;
    QPushButton *finishDrawButton;

    void setupUi(QWidget *ControlFrame)
    {
        if (ControlFrame->objectName().isEmpty())
            ControlFrame->setObjectName("ControlFrame");
        ControlFrame->resize(300, 149);
        gridLayout_2 = new QGridLayout(ControlFrame);
        gridLayout_2->setObjectName("gridLayout_2");
        label_4 = new QLabel(ControlFrame);
        label_4->setObjectName("label_4");

        gridLayout_2->addWidget(label_4, 0, 2, 1, 1);

        translateSlider = new QSlider(ControlFrame);
        translateSlider->setObjectName("translateSlider");
        translateSlider->setMaximum(100);
        translateSlider->setOrientation(Qt::Orientation::Horizontal);

        gridLayout_2->addWidget(translateSlider, 1, 3, 1, 1);

        rotateSlider = new QSlider(ControlFrame);
        rotateSlider->setObjectName("rotateSlider");
        rotateSlider->setMaximum(360);
        rotateSlider->setOrientation(Qt::Orientation::Horizontal);

        gridLayout_2->addWidget(rotateSlider, 0, 1, 1, 1);

        label_3 = new QLabel(ControlFrame);
        label_3->setObjectName("label_3");

        gridLayout_2->addWidget(label_3, 1, 0, 1, 1);

        label_2 = new QLabel(ControlFrame);
        label_2->setObjectName("label_2");

        gridLayout_2->addWidget(label_2, 1, 2, 1, 1);

        scaleSlider = new QSlider(ControlFrame);
        scaleSlider->setObjectName("scaleSlider");
        scaleSlider->setMaximum(100);
        scaleSlider->setSingleStep(1);
        scaleSlider->setOrientation(Qt::Orientation::Horizontal);

        gridLayout_2->addWidget(scaleSlider, 0, 3, 1, 1);

        shearSlider = new QSlider(ControlFrame);
        shearSlider->setObjectName("shearSlider");
        shearSlider->setMaximum(20);
        shearSlider->setOrientation(Qt::Orientation::Horizontal);

        gridLayout_2->addWidget(shearSlider, 1, 1, 1, 1);

        label_1 = new QLabel(ControlFrame);
        label_1->setObjectName("label_1");

        gridLayout_2->addWidget(label_1, 0, 0, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        exportButton = new QPushButton(ControlFrame);
        exportButton->setObjectName("exportButton");

        gridLayout->addWidget(exportButton, 1, 0, 1, 1);

        addWatermarkButton = new QPushButton(ControlFrame);
        addWatermarkButton->setObjectName("addWatermarkButton");

        gridLayout->addWidget(addWatermarkButton, 1, 1, 1, 1);

        drawButton = new QPushButton(ControlFrame);
        drawButton->setObjectName("drawButton");

        gridLayout->addWidget(drawButton, 1, 2, 1, 1);

        drawPanel = new QWidget(ControlFrame);
        drawPanel->setObjectName("drawPanel");
        horizontalLayout_draw = new QHBoxLayout(drawPanel);
        horizontalLayout_draw->setObjectName("horizontalLayout_draw");
        horizontalLayout_draw->setContentsMargins(0, 0, 0, 0);
        drawToolCombo = new QComboBox(drawPanel);
        drawToolCombo->addItem(QString());
        drawToolCombo->addItem(QString());
        drawToolCombo->setObjectName("drawToolCombo");

        horizontalLayout_draw->addWidget(drawToolCombo);

        finishDrawButton = new QPushButton(drawPanel);
        finishDrawButton->setObjectName("finishDrawButton");

        horizontalLayout_draw->addWidget(finishDrawButton);


        gridLayout->addWidget(drawPanel, 2, 0, 1, 3);


        gridLayout_2->addLayout(gridLayout, 2, 0, 1, 4);

        QWidget::setTabOrder(rotateSlider, scaleSlider);
        QWidget::setTabOrder(scaleSlider, shearSlider);
        QWidget::setTabOrder(shearSlider, translateSlider);
        QWidget::setTabOrder(translateSlider, exportButton);

        retranslateUi(ControlFrame);

        QMetaObject::connectSlotsByName(ControlFrame);
    } // setupUi

    void retranslateUi(QWidget *ControlFrame)
    {
        ControlFrame->setWindowTitle(QCoreApplication::translate("ControlFrame", "Form", nullptr));
        label_4->setText(QCoreApplication::translate("ControlFrame", "\347\274\251\346\224\276", nullptr));
        label_3->setText(QCoreApplication::translate("ControlFrame", "\345\210\207\345\217\230", nullptr));
        label_2->setText(QCoreApplication::translate("ControlFrame", "\344\275\215\347\247\273", nullptr));
        label_1->setText(QCoreApplication::translate("ControlFrame", "\346\227\213\350\275\254", nullptr));
        exportButton->setText(QCoreApplication::translate("ControlFrame", "\345\257\274\345\207\272\345\233\276\347\211\207", nullptr));
        addWatermarkButton->setText(QCoreApplication::translate("ControlFrame", "\346\267\273\345\212\240\346\260\264\345\215\260", nullptr));
        drawButton->setText(QCoreApplication::translate("ControlFrame", "\347\273\230\345\210\266", nullptr));
        drawToolCombo->setItemText(0, QCoreApplication::translate("ControlFrame", "\347\237\251\345\275\242", nullptr));
        drawToolCombo->setItemText(1, QCoreApplication::translate("ControlFrame", "\347\233\264\347\272\277", nullptr));

        finishDrawButton->setText(QCoreApplication::translate("ControlFrame", "\345\256\214\346\210\220", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ControlFrame: public Ui_ControlFrame {};
} // namespace Ui

QT_END_NAMESPACE

#endif // _UIC_TEST_UI_CONTROLFRAME_H
