#include "MSlider.h"
#include <QMouseEvent>
MSlider::MSlider(QWidget* parent) :QSlider(parent)
{

}

MSlider::~MSlider()
{

}

void MSlider::mousePressEvent(QMouseEvent* ev)
{
    int currentX = ev->pos().x();
    double per = currentX * 1.0 / this->width();
    int value = per * (this->maximum() - this->minimum()) + this->minimum();
    this->setValue(value);
    QSlider::mousePressEvent(ev);
}