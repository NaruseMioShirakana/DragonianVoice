#pragma once
#include <QScrollArea>

class MScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    explicit MScrollArea(QWidget* parent = nullptr);
    ~MScrollArea() override = default;
    bool eventFilter(QObject* watched, QEvent* event) override;
};

