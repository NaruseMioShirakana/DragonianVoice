#include "svsmainwindow.h"
#include "ui_svsmainwindow.h"

SVSMainWindow::SVSMainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVSMainWindow)
{
    ui->setupUi(this);
}

SVSMainWindow::~SVSMainWindow()
{
    delete ui;
}
