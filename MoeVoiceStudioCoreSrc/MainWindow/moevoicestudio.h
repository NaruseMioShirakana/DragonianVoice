#ifndef MOEVOICESTUDIO_H
#define MOEVOICESTUDIO_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MoeVoiceStudio; }
QT_END_NAMESPACE

class MoeVoiceStudio : public QMainWindow
{
    Q_OBJECT

public:
    MoeVoiceStudio(QWidget *parent = nullptr);
    ~MoeVoiceStudio();

private:
    Ui::MoeVoiceStudio *ui;
};
#endif // MOEVOICESTUDIO_H
