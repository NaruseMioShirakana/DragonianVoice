#ifndef SVCINFERSLICERSETTING_H
#define SVCINFERSLICERSETTING_H

#include <QWidget>
#include <QCloseEvent>

#include "ui_svcinferslicersetting.h"

namespace Ui {
class SvcInferSlicerSetting;
}

class SvcInferSlicerSetting : public QWidget
{
    Q_OBJECT

public:
    explicit SvcInferSlicerSetting(const std::function<void(double, long, long, long)>& _cb, QWidget *parent = nullptr);
    ~SvcInferSlicerSetting() override;
    void closeEvent(QCloseEvent* event) override
    {
        _callback(ui->SvcInferSlicerSettingHopSizeSpinBox->value(), ui->SvcInferSlicerSettingMinLengthSpinBox->value(), ui->SvcInferSlicerSettingWindowSizeSpinBox->value(), ui->SvcInferSlicerSettingHopSizeSpinBox->value());
        event->accept();
    }
private:
	const std::function<void(double, long, long, long)>& _callback;
    Ui::SvcInferSlicerSetting *ui;
};

#endif // SVCINFERSLICERSETTING_H
