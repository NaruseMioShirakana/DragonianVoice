#ifndef TTSEDITOR_H
#define TTSEDITOR_H

#include <QWidget>
namespace Ui {
class TTSEditor;
}

class TTSEditor : public QWidget
{
    Q_OBJECT

public:
    explicit TTSEditor(QWidget *parent = nullptr);
    ~TTSEditor() override;
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    Ui::TTSEditor *ui;
};

#endif // TTSEDITOR_H
