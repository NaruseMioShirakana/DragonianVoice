#pragma once
#include <QWidget>

class waveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit waveWidget(QWidget* parent = nullptr);
    ~waveWidget() override;
    void setWavFile(const std::wstring& path);
    void setPenColor(int R,int G,int B)
    {
        penColor = QColor(R, G, B);
        update();
    }
    void setBackgroundColor(int R, int G, int B)
    {
        backgroundColor = QColor(R, G, B);
        update();
    }
protected:
	void paintEvent(QPaintEvent* event) override;
    void drawPCMData();
private:
    void drawWave(QPainter& p, int W, int H) const;
    QColor backgroundColor = QColor(40, 40, 40);
    QColor penColor = QColor(0, 191, 255);
    QColor lineColor = QColor(255, 255, 0);
    // ²¨ÐÎÍ¼½âÎö
    std::vector<int16_t> m_Wavefile = std::vector<int16_t>(10000, 0);
};

class spectrogramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit spectrogramWidget(QWidget* parent = nullptr);
    ~spectrogramWidget() override;
    void setWavFile(const std::wstring& path);
    void setPenColor(int R, int G, int B)
    {
        penColor = QColor(R, G, B);
        update();
    }
    void setBackgroundColor(int R, int G, int B)
    {
        backgroundColor = QColor(R, G, B);
        update();
    }
protected:
    void paintEvent(QPaintEvent* event) override;
    void getImage(std::vector<std::vector<double>> m_SpecData);
    void drawSpecData();
	std::vector<QColor> colorMap =
    {
        QColor(int(0.0f * 255), int(0.0f * 255),int(0.0f * 255)),       // black
        QColor(int(0.0f * 255), int(0.0f * 255),int(0.251f * 255)),     // dark blue
        QColor(int(0.0f * 255), int(0.0f * 255),int(0.502f * 255)),     // blue
        QColor(int(0.251f * 255), int(0.0f * 255),int(0.502f * 255)),   // purple-blue
        QColor(int(0.502f * 255),int(0.0f * 255),int(0.502f * 255)),   // purple
        QColor(int(0.753f * 255),int(0.0f * 255),int(0.502f * 255)),   // magenta
        QColor(int(1.0f * 255),int(0.0f * 255),int(0.0f * 255)),       // red
        QColor(int(1.0f * 255),int(0.502f * 255),int(0.0f * 255)),     // orange
        QColor(int(1.0f * 255),int(1.0f * 255),int(0.0f * 255)),       // yellow
        QColor(int(1.0f * 255),int(1.0f * 255),int(0.502f * 255)),     // light yellow
    };
private:
    QColor backgroundColor = QColor(40, 40, 40);
    QColor penColor = QColor(0, 191, 255);
    QColor lineColor = QColor(255, 255, 0);
    //std::vector<std::vector<double>> m_SpecData;
    QImage imageSpec = QImage(QSize(1024, 768), QImage::Format_ARGB32);
};