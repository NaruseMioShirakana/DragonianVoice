#ifndef MOEVOICESTUDIO_H
#define MOEVOICESTUDIO_H

#include <QMainWindow>
#include "mainmenu.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MoeVoiceStudio; }
QT_END_NAMESPACE

class MoeVoiceStudio : public QMainWindow
{
    Q_OBJECT
public:
    MoeVoiceStudio(QWidget *parent = nullptr);
    ~MoeVoiceStudio() override;
    void paintEvent(QPaintEvent* event) override;
public slots:
    void on_DisclaimerPageAgreeButton_clicked();
    void on_DisclaimerPageRefuseButton_clicked();
private:
    Ui::MoeVoiceStudio *ui;
    MainMenu _mainmenu;
};
#endif // MOEVOICESTUDIO_H
