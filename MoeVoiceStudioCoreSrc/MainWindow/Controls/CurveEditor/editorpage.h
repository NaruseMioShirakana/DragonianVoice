#pragma once
#include "qcustomplot.h"
#include <QQueue>


class MPitchLabel : public QCPAxisTicker
{

public: 

    QString getTickLabel(double tick, const QLocale& locale, QChar formatChar, int precision) override
    {
        const auto pitch = int(round(tick));
        if (pitch < 0)
            return "Null";
        return QString(Labels[pitch % 12]) + std::to_string(pitch / 12).c_str();
    }

protected:
    constexpr static const char* Labels[12] = { "C","C#" ,"D" ,"D#" ,"E" ,"F" ,"F#" ,"G" ,"G#" ,"A" ,"A#" ,"B" };
};

class MCurveEditorPage : public QCustomPlot
{
    Q_OBJECT

public:
    explicit MCurveEditorPage(QWidget* parent = nullptr) :QCustomPlot(parent)
    {
        setInteractions(QCP::iSelectPlottables | QCP::iRangeDrag);
        PlayProgress = new QCPItemTracer(this);
        PlayProgressPen.setColor(QColor(Qt::black));
        PlayProgressPen.setStyle(Qt::SolidLine);
        PlayProgress->setPen(PlayProgressPen);
        PlayProgress->setVisible(false);
        PlayProgress->position->setCoords(0,0);

        HPlayProgress = new QCPItemTracer(this);
        HPlayProgress->setPen(PlayProgressPen);
        HPlayProgress->setVisible(true);
        HPlayProgress->position->setCoords(0, 0);
    }
    ~MCurveEditorPage() override = default;
    void LinearCombination(int idx, double Value = 1.0) const
    {
        if(graphCount() == 1)
        {
            if (idx < graph(0)->dataCount())
                (graph(0)->data()->begin() + idx)->value = 1.0;
            return;
        }
        double Sum = 0.0;
        for (int i = 0; i < graphCount(); ++i)
            if (idx < graph(i)->dataCount())
                Sum += (graph(i)->data()->begin() + idx)->value;
        if (Sum < 0.0001)
        {
            graph(0)->data()->begin()->value = 1.0;
            return;
        }
        Sum *= Value;
        for (int i = 0; i < graphCount(); ++i)
            (graph(i)->data()->begin() + idx)->value /= Sum;
    }
    void enableChara()
    {
        chara_enable = true;
    }
    void clearBackup()
    {
        _queuectrlz.clear();
        _queuectrly.clear();
    }
    void setProgress(double pos)
    {
        PlayProgress->position->setCoords(pos, 0);
        replot();
    }
    void ProgressEnabled(bool _cond)
    {
        PlayProgress->setVisible(_cond);
        replot();
    }
    void setCallback(const std::function<void(double)>& _CB)
    {
        _CallBack = _CB;
    }
    /***********InitFn************/


    /***********Update************/
    void changeGraph(int idx)
    {
        cur_cha = idx;
    }

    void setMaxValue(double _val)
    {
        max_value = _val;
    }

    /***********Event*************/
    void wheelEvent(QWheelEvent* event) override
    {
        if(event->modifiers() == Qt::ControlModifier)
        {
            const auto deltay = event->angleDelta().y();
            const auto cur_range = xAxis->range();
            const auto R = xAxis->range().size() / 2;
            if (deltay > 0)
                xAxis->setRange(cur_range.lower + R * 0.1, cur_range.upper - R * 0.1);
            else if(deltay < 0)
                xAxis->setRange(cur_range.lower - R * 0.1, cur_range.upper + R * 0.1);
            return;
        }
    	if(event->modifiers() == Qt::AltModifier)
        {
            const auto deltay = event->angleDelta().x();
            const auto cur_range = yAxis->range();
            const auto R = yAxis->range().size() / 2;
            if (deltay > 0)
                yAxis->setRange(cur_range.lower + R * 0.1, cur_range.upper - R * 0.1);
            else if (deltay < 0)
                yAxis->setRange(cur_range.lower - R * 0.1, cur_range.upper + R * 0.1);
            return;
        }
    	if(event->modifiers() == Qt::NoModifier)
        {
            const auto deltay = event->angleDelta().y();
            const auto cur_range = yAxis->range();
            const auto R = yAxis->range().size() / 2;
            if (deltay > 0)
                yAxis->setRange(cur_range.lower + R * 0.5, cur_range.upper + R * 0.5);
            else if (deltay < 0)
                yAxis->setRange(cur_range.lower - R * 0.5, cur_range.upper - R * 0.5);
            return;
        }
        QCustomPlot::wheelEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->buttons() & Qt::LeftButton)
        {
            if (graph(cur_cha) && graph(cur_cha)->dataCount() && event->modifiers() == Qt::AltModifier)
            {
                _queuectrly.clear();
                if (_queuectrlz.size() > 100)
                    _queuectrlz.pop_front();
                QVector<double> _backupData;
                _backupData.reserve(graph(cur_cha)->dataCount());
                for(const auto i : *graph(cur_cha)->data())
                    _backupData.emplace_back(i.value);
                _queuectrlz.enqueue({ cur_cha , std::move(_backupData) });
                return;
            }
            if (event->modifiers() == Qt::ShiftModifier)
            {
                _CallBack(xAxis->pixelToCoord(event->pos().x()));
                return;
            }
        }
        QCustomPlot::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
	    if((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_Z))
	    {
            if (_queuectrlz.empty())
                return;
            if (graph(cur_cha) && graph(cur_cha)->dataCount())
            {
                QVector<double> _keys(_queuectrlz.back().second.size());
                for (int64_t i = 0; i < _keys.size(); ++i)
                    _keys[i] = double(i);
                {
                    if (_queuectrly.size() > 100)
                        _queuectrly.pop_front();
                    QVector<double> _backupData;
                    _backupData.reserve(graph(cur_cha)->dataCount());
                    for (const auto i : *graph(cur_cha)->data())
                        _backupData.emplace_back(i.value);
                    _queuectrly.enqueue({ cur_cha , std::move(_backupData) });
                }
                graph(_queuectrlz.back().first)->setData(_keys, _queuectrlz.back().second);
                _queuectrlz.pop_back();
                replot();
            }
            return;
	    }
    	if((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_Y))
        {
            if (_queuectrly.empty())
                return;
            QVector<double> _keys(_queuectrly.back().second.size());
            for (int64_t i = 0; i < _keys.size(); ++i)
                _keys[i] = double(i);
            {
                if (_queuectrlz.size() > 100)
                    _queuectrlz.pop_front();
                QVector<double> _backupData;
                _backupData.reserve(graph(cur_cha)->dataCount());
                for (const auto i : *graph(cur_cha)->data())
                    _backupData.emplace_back(i.value);
                _queuectrlz.enqueue({ cur_cha , std::move(_backupData) });
            }
            graph(_queuectrly.back().first)->setData(_keys, _queuectrly.back().second);
            _queuectrly.pop_back();
            replot();
            return;
        }
        QCustomPlot::keyPressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
	    if(event->buttons() & Qt::LeftButton)
	    {
            if(event->modifiers() == Qt::AltModifier)
            {
                const int x_pos = event->pos().x();
                const int y_pos = event->pos().y();
                const int x_val = int(xAxis->pixelToCoord(x_pos));
                if (graph(cur_cha) && graph(cur_cha)->dataCount() && x_val < graph(cur_cha)->dataCount() && x_val > -1)
                {
	                double y_val = yAxis->pixelToCoord(y_pos);
                	if (y_val < 0)
                		y_val = 0;
                	if (y_val > max_value)
                		y_val = max_value;
                	(graph(cur_cha)->data()->begin() + x_val)->value = y_val;
                    if(chara_enable)
						LinearCombination(x_val);
                	replot();
                    return;
                }
            }
	    }
        QCustomPlot::mouseMoveEvent(event);
    }
private:
    constexpr static QColor _ColorMap[2] = { QColor(192,192,192),QColor(169,169,169) };
    QQueue<std::pair<int, QVector<double>>> _queuectrlz;
    QQueue<std::pair<int, QVector<double>>> _queuectrly;
    bool LeftPressed = false;
    int cur_cha = 0;
    double max_value = 128;
    bool chara_enable = false;
    QImage BG;
    QCPItemTracer* PlayProgress = nullptr;
    QCPItemTracer* HPlayProgress = nullptr;
    QPen PlayProgressPen;
    std::function<void(double)> _CallBack;
};