#ifndef SVCMAINWINDOW_H
#define SVCMAINWINDOW_H

#include <QDialog>

namespace Ui {
class SVCMainWindow;
}

class SVCMainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SVCMainWindow(QWidget *parent = nullptr);
    ~SVCMainWindow() override;

private:
    Ui::SVCMainWindow *ui;
};

#endif // SVCMAINWINDOW_H
