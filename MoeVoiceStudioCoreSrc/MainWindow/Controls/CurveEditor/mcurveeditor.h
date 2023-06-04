#ifndef MCURVEEDITOR_H
#define MCURVEEDITOR_H

#include <QWidget>

namespace Ui {
class MCurveEditor;
}

class MCurveEditor : public QWidget
{
    Q_OBJECT

public:
    explicit MCurveEditor(QWidget *parent = nullptr);
    ~MCurveEditor() override;
    /***********InitFn************/
    void initScrollAreas();
    /***********Update************/
    

    /***********Event*************/
    void wheelEvent(QWheelEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    Ui::MCurveEditor *ui;
};

#endif // MCURVEEDITOR_H
