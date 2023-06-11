#include "svcinferslicersetting.h"
#include "ui_svcinferslicersetting.h"

SvcInferSlicerSetting::SvcInferSlicerSetting(const std::function<void(double, long, long, long)>& _cb, QWidget *parent) :
    QWidget(parent),
    _callback(_cb),
    ui(new Ui::SvcInferSlicerSetting)
{
    ui->setupUi(this);
}

SvcInferSlicerSetting::~SvcInferSlicerSetting()
{
    delete ui;
}
