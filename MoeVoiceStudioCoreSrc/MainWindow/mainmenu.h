#ifndef MAINMENU_H
#define MAINMENU_H

#include <QDialog>
#include "TTS/ttsmainwindow.h"
#include "SVS/svsmainwindow.h"
#include "SVC/svcmainwindow.h"
#include "moevssetting.h"

namespace Ui {
class MainMenu;
}

class MainMenu : public QDialog
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);
    ~MainMenu() override;
    std::function<void(QDialog*)> _close_callback = [&](QDialog* _tts_window)
    {
        _tts_window->hide();
        show();
    };

public slots:
    void on_MainMenuExitButton_clicked();
    void on_MainMenuInfoAndConfigButton_clicked();
    void on_MainMenuSVCButton_clicked();
    void on_MainMenuSVSButton_clicked();
    void on_MainMenuTTSButton_clicked();
private:
    Ui::MainMenu *ui;
    SVCMainWindow _svc{ _close_callback };
    SVSMainWindow _svs{ _close_callback };
    TTSMainWindow _tts{ _close_callback };
};

#endif // MAINMENU_H
