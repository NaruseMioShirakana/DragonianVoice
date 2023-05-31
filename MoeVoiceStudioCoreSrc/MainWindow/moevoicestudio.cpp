#include "moevoicestudio.h"
#include "./ui_moevoicestudio.h"
#include "QPainter"
#include "QMessageBox"

MoeVoiceStudio::MoeVoiceStudio(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MoeVoiceStudio)
{
    ui->setupUi(this);
    ui->retranslateUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
}

MoeVoiceStudio::~MoeVoiceStudio()
{
    delete ui;
}

void MoeVoiceStudio::paintEvent(QPaintEvent* event)
{
    //ui->DisclaimerPageBackGround->setPixmap(QPixmap(":/image/bg/bg_sq_small_1"));
}

void MoeVoiceStudio::on_DisclaimerPageAgreeButton_clicked()
{
    hide();
    _mainmenu.show();
}

void MoeVoiceStudio::on_DisclaimerPageRefuseButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("DisclaimerPageMsgBoxTitle"));
    msgBox.setText(tr("DisclaimerPageMsgBoxText"));
    msgBox.setInformativeText(tr("DisclaimerPageMsgBoxInformativeText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("DisclaimerPageMsgBoxButtonYes"));
    msgBox.button(QMessageBox::No)->setText(tr("DisclaimerPageMsgBoxButtonNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
	const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
        close();
}