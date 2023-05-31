#ifndef SVSMAINWINDOW_H
#define SVSMAINWINDOW_H

#include <QDialog>

namespace Ui {
class SVSMainWindow;
}

class SVSMainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SVSMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent = nullptr);
    ~SVSMainWindow() override;

private:
    Ui::SVSMainWindow *ui;
    const std::function<void(QDialog*)>& exit_callback;
};

#endif // SVSMAINWINDOW_H
