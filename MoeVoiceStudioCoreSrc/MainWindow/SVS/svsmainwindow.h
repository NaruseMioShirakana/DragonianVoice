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
    explicit SVSMainWindow(QWidget *parent = nullptr);
    ~SVSMainWindow();

private:
    Ui::SVSMainWindow *ui;
};

#endif // SVSMAINWINDOW_H
