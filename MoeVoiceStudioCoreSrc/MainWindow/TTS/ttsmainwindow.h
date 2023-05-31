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
    explicit TTSMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent = nullptr);
    ~TTSMainWindow() override;

private:
    Ui::TTSMainWindow *ui;
    const std::function<void(QDialog*)>& exit_callback;
};

#endif // TTSMAINWINDOW_H
