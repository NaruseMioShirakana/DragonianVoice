#include "svcmainwindow.h"
#include "ui_svcmainwindow.h"

SVCMainWindow::SVCMainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVCMainWindow)
{
    ui->setupUi(this);
}

SVCMainWindow::~SVCMainWindow()
{
    delete ui;
}
