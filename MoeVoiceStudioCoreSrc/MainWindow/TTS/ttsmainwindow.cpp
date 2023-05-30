#include "ttsmainwindow.h"
#include "ui_ttsmainwindow.h"

TTSMainWindow::TTSMainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TTSMainWindow)
{
    ui->setupUi(this);
}

TTSMainWindow::~TTSMainWindow()
{
    delete ui;
}
