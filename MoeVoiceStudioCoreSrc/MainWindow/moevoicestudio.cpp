#include "moevoicestudio.h"
#include "./ui_moevoicestudio.h"

MoeVoiceStudio::MoeVoiceStudio(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MoeVoiceStudio)
{
    ui->setupUi(this);
    ui->retranslateUi(this);
}

MoeVoiceStudio::~MoeVoiceStudio()
{
    delete ui;
}

