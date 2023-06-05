#include "meditor.h"

MTTSEditor::MTTSEditor(QWidget* parent) : QWidget(parent)
{
    totalDuration = (double)width();
}

void MTTSEditor::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    const auto mheight = height(), mwidth = width();
    BlockHeight = mheight / 8;
    for (int i = 0; i < 8; ++i)
        if (i % 2)
            painter.fillRect(QRect(QPoint(0, int(BeginPointPos[i] * mheight)) , QPoint(mwidth, int(BeginPointPos[i + 1] * mheight))), { 40,40,40 });
        else
            painter.fillRect(QRect(QPoint(0, int(BeginPointPos[i] * mheight)), QPoint(mwidth, int(BeginPointPos[i + 1] * mheight))), { 192,192,192 });
    for (size_t i = 0; i < _blocks.size(); ++i)
    {
        const auto durpc = _durations[i] / totalDuration;
        _blocks[i]->setFixedWidth(int(durpc * width()));
        _blocks[i]->setFixedHeight(BlockHeight);
        _blocks[i]->update();
    }
    update();
}