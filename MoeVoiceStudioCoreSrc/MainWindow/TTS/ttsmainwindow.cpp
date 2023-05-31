#include "ttsmainwindow.h"
#include "ui_ttsmainwindow.h"

TTSMainWindow::TTSMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TTSMainWindow),
    exit_callback(_callback)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
}

TTSMainWindow::~TTSMainWindow()
{
    delete ui;
}
