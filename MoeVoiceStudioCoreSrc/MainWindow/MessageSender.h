#pragma once
#include <QObject>

class MessageSender : public QObject
{
	Q_OBJECT
public:
    void InferProcess(size_t cur, size_t all)
    {
        emit onProcessBarChanging(cur, all);     //通过emit关键字发射信号
    }
    void ChangeInferenceStat(bool condition)
    {
        emit onAnyStatChanging(condition);
    }
    void ChangeAnyStat(bool condition)
    {
        emit onAnyStatChanging(condition);
    }
    void strParamEvent(std::wstring _str)
    {
        emit strParamedEvent(_str);
    }
    void astrParamEvent(std::string _str)
    {
        emit astrParamedEvent(_str);
    }
    void mPaintEventSender()
    {
        emit mPaintEvent();
    }

signals: void onProcessBarChanging(size_t, size_t);
signals: void onAnyStatChanging(bool);
signals: void strParamedEvent(std::wstring);
signals: void astrParamedEvent(std::string);
signals: void mPaintEvent();
};