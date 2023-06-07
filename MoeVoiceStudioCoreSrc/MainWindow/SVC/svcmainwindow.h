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
    explicit SVCMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent = nullptr);
    ~SVCMainWindow() override;

private:
    Ui::SVCMainWindow *ui;
    const std::function<void(QDialog*)>& exit_callback;
    //QCustomPlot* F0;
};

#endif // SVCMAINWINDOW_H
