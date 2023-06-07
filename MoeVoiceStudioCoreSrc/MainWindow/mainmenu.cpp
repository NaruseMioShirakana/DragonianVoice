#include "mainmenu.h"

#include <QMessageBox>

#include "ui_mainmenu.h"

MainMenu::MainMenu(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainMenu)
{
    ui->setupUi(this);
}

MainMenu::~MainMenu()
{
    delete ui;
}

void MainMenu::on_MainMenuTTSButton_clicked()
{
    _tts.show();
    hide();
}

void MainMenu::on_MainMenuSVCButton_clicked()
{
    _svc.show();
    hide();
}

void MainMenu::on_MainMenuSVSButton_clicked()
{
    //TODO
    //_svs.show();
    //hide();
}

void MainMenu::on_MainMenuInfoAndConfigButton_clicked()
{

}

void MainMenu::on_MainMenuExitButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("MainMenuExitButtonTitle"));
    msgBox.setText(tr("MainMenuExitButtonText"));
    msgBox.setInformativeText(tr("MainMenuExitButtonInformativeText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("MainMenuExitButtonButtonYes"));
    msgBox.button(QMessageBox::No)->setText(tr("MainMenuExitButtonButtonNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
        close();
}