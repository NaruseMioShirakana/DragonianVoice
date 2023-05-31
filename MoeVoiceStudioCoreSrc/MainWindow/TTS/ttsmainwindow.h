#ifndef TTSMAINWINDOW_H
#define TTSMAINWINDOW_H

#include <QDialog>

namespace Ui {
class TTSMainWindow;
}

class TTSMainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TTSMainWindow(QWidget *parent = nullptr);
    ~TTSMainWindow() override;

private:
    Ui::TTSMainWindow *ui;
};

#endif // TTSMAINWINDOW_H
