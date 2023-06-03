#pragma once
#include <QObject>
#include <QSlider>
class MSlider : public QSlider
{
public:
    MSlider(QWidget* parent = nullptr);
    ~MSlider() override;
    void mousePressEvent(QMouseEvent* ev) override;
};