#ifndef MOEVSSETTING_H
#define MOEVSSETTING_H

#include <QWidget>

namespace Ui {
class MoeVSSetting;
}

class MoeVSSetting : public QWidget
{
    Q_OBJECT

public:
    explicit MoeVSSetting(QWidget *parent = nullptr);
    ~MoeVSSetting() override;
    void closeEvent(QCloseEvent* event) override;
public slots:
    void on_SettingProvicerComboBox_currentIndexChanged(int value);
    void on_SettingNumThreadComboBox_currentIndexChanged(int value);
    void on_SettingDeviceIDComboBox_currentIndexChanged(int value);

private:
    Ui::MoeVSSetting *ui;
};

#endif // MOEVSSETTING_H
