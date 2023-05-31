#ifndef MAINMENU_H
#define MAINMENU_H

#include <QDialog>
#include "SVC/svcmainwindow.h"
#include "TTS/ttsmainwindow.h"
#include "SVS/svsmainwindow.h"

namespace Ui {
class MainMenu;
}

class MainMenu : public QDialog
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);
    ~MainMenu() override;
public slots:
    void on_MainMenuExitButton_clicked();
    void on_MainMenuInfoAndConfigButton_clicked();
    void on_MainMenuSVCButton_clicked();
    void on_MainMenuSVSButton_clicked();
    void on_MainMenuTTSButton_clicked();
private:
    Ui::MainMenu *ui;
    SVCMainWindow _svc;
    SVSMainWindow _svs;
    TTSMainWindow _tts;
};

#endif // MAINMENU_H
