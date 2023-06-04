#include "mcurveeditor.h"
#include "ui_mcurveeditor.h"
#include <QWheelEvent>
#include <QScrollBar>

MCurveEditor::MCurveEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MCurveEditor)
{
    ui->setupUi(this);

    ui->MCurveEditorScroll->installEventFilter(this);
}

MCurveEditor::~MCurveEditor()
{
    delete ui;
}

void setControlWidth(QWidget* _obj, int _p, int _num_plus = 20)
{
    if (_obj->maximumWidth() < _num_plus * 4 && _p < 0)
        return;
    if (_obj->maximumWidth() > 999999 - (_num_plus * 4) && _p > 0)
        return;
    _p = _p < 0 ? -1 : 1;
    _obj->setMaximumWidth(_obj->maximumWidth() + _num_plus * _p);
    _obj->setMinimumWidth(_obj->minimumWidth() + _num_plus * _p);
    _obj->setFixedWidth(_obj->width() + _num_plus * _p);
}

void setControlHeight(QWidget* _obj, int _p, int _num_plus = 20)
{
    if (_obj->maximumHeight() < _num_plus * 4 && _p < 0)
        return;
    if (_obj->maximumHeight() > 999999 - (_num_plus * 4) && _p > 0)
        return;
    _p = _p < 0 ? -1 : 1;
    _obj->setMaximumHeight(_obj->maximumHeight() + _num_plus * _p);
    _obj->setMinimumHeight(_obj->minimumHeight() + _num_plus * _p);
    _obj->setFixedHeight(_obj->height() + _num_plus * _p);
}

void setControlRect(QWidget* _obj, QWidget* _obj2)
{
    _obj->setMaximumHeight(_obj2->maximumHeight());
    _obj->setMinimumHeight(_obj2->minimumHeight());
    _obj->setFixedHeight(_obj2->height());
    _obj->setMaximumWidth(_obj2->maximumWidth());
    _obj->setMinimumWidth(_obj2->minimumWidth());
    _obj->setFixedWidth(_obj2->width());
}

void MCurveEditor::wheelEvent(QWheelEvent* event)
{
    const QRectF rectTotalScrollArea = ui->MCurveEditorCurveArea->geometry();
    const QRectF rectTotalScrollF0 = ui->MCurveEditorF0->geometry();
    const QRectF rectTotalScrollChara = ui->MCurveEditorCharaMix->geometry();
    const QRectF rectTotalScrolVolume = ui->MCurveEditorVolume->geometry();
    if(rectTotalScrollArea.contains(event->position()))
    {
        //const auto numPixels = event->pixelDelta();
    	auto numDegrees = event->angleDelta();
        if (numDegrees.isNull())
            numDegrees = event->pixelDelta();
        if (numDegrees.isNull())
            return;
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
        {
            //横向缩放
            setControlWidth(ui->MCurveEditorF0, numDegrees.y());
            setControlWidth(ui->MCurveEditorCharaMix, numDegrees.y());
            setControlWidth(ui->MCurveEditorVolume, numDegrees.y());
        }
        else if (QApplication::keyboardModifiers() == Qt::ShiftModifier)
        {
            //横向移动
        }
        else if (QApplication::keyboardModifiers() == Qt::AltModifier)
        {
            if(rectTotalScrollF0.contains(event->position()))
                setControlHeight(ui->MCurveEditorF0, numDegrees.y());
            else if (rectTotalScrollChara.contains(event->position()))
                setControlHeight(ui->MCurveEditorCharaMix, numDegrees.y());
            else if (rectTotalScrolVolume.contains(event->position()))
                setControlHeight(ui->MCurveEditorVolume, numDegrees.y());
            //纵向缩放
        }
        else if (QApplication::keyboardModifiers() == Qt::NoModifier)
        {
            //纵向移动
            return;
            if (rectTotalScrollF0.contains(event->position()))
            {
	            setControlHeight(ui->MCurveEditorF0, numDegrees.y());
            }
            else if (rectTotalScrollChara.contains(event->position()))
            {
	            setControlHeight(ui->MCurveEditorCharaMix, numDegrees.y());
            }
            else if (rectTotalScrolVolume.contains(event->position()))
            {
	            setControlHeight(ui->MCurveEditorVolume, numDegrees.y());
            }
        }
        else
            event->ignore();
    }
}

bool MCurveEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->MCurveEditorScroll)
        if(event->type() == QEvent::Scroll)
            return true;
    return false;
}
