#include "svsmainwindow.h"
#include "ui_svsmainwindow.h"

SVSMainWindow::SVSMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVSMainWindow),
    exit_callback(_callback)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
}

SVSMainWindow::~SVSMainWindow()
{
    delete ui;
}
