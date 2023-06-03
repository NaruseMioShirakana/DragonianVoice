#include "player.h"
#include <QPainter>
#include "AvCodeResample.h"
#include "../../../Lib/fftw/api/fftw3.h"

waveWidget::waveWidget(QWidget* parent) :
	QWidget(parent)
{
}

waveWidget::~waveWidget() = default;

void waveWidget::setWavFile(const std::wstring& path)
{
	try
	{
		m_Wavefile = AudioPreprocess().codec(path, 32000);
	}
	catch (std::exception& e)
	{
		m_Wavefile = std::vector(4000, 0i16);
	}
	update();
}

void waveWidget::paintEvent(QPaintEvent* e)
{
	drawPCMData();
}

void waveWidget::drawPCMData()
{
	const int W = this->width();
	const int H = this->height();

	QPixmap pix(W, H);

	const QPalette palette;
	const auto color = palette.color(QPalette::Window);
	pix.fill(color);

	QPainter p(&pix);
	QPen pen;
	pen.setColor(penColor);
	p.setPen(pen);
	QPainter painter(this);
	drawWave(p, W, H);
	painter.drawPixmap(0, 0, pix);
}

void waveWidget::drawWave(QPainter& p, int W, int H) const
{
	p.fillRect(this->rect(), backgroundColor);
	p.setPen(QPen(lineColor, 0.2, Qt::SolidLine));
	p.drawLine(0, int(height() * 0.05), this->width(), int(height() * 0.05));
	p.drawLine(0, int(height() * 0.95), this->width(), int(height() * 0.95));
	p.setPen(QPen(lineColor, 0.5, Qt::SolidLine));
	p.drawLine(0, int(height() * 0.25), this->width(), int(height() * 0.25));
	p.drawLine(0, int(height() * 0.75), this->width(), int(height() * 0.75));
	p.setPen(QPen(penColor));
	const double A = pow(2.0, 8.0 * 2 - 1);
	const int y = H / 2;
	int x = 0;
	while (x < W)
	{
		int min = INT_MAX, max = INT_MIN;
		for (int j = x * ((long)m_Wavefile.size() / W); j < (x + 1) * ((long)m_Wavefile.size() / W); j++)
		{
			if (j > 0 && j < (long)m_Wavefile.size())
			{
				if (m_Wavefile[j] < min)
					min = m_Wavefile[j];
				if (m_Wavefile[j] > max)
					max = m_Wavefile[j];
			}
		}
		p.drawLine(x, y + (long)(max * H / 2.0 / A), x, y + (long)(min * H / 2.0 / A));
		x++;
	}
}

constexpr double PI = 3.14159265358979323846;

void HannWindow(double* data, int size) {
    for (int i = 0; i < size; i++) {
        const double windowValue = 0.5 * (1 - cos(2 * PI * i / (size - 1)));
        data[i] *= windowValue;
    }
}

void ConvertDoubleToFloat(const std::vector<double>& input, float* output)
{
    for (size_t i = 0; i < input.size(); i++) {
        output[i] = static_cast<float>(input[i]);
    }
}

void CalculatePowerSpectrum(double* real, const double* imag, int size) {
    for (int i = 0; i < size; i++) {
        real[i] = real[i] * real[i] + imag[i] * imag[i];
    }
}

void ConvertPowerSpectrumToDecibels(double* data, int size) {
    for (int i = 0; i < size; i++) {
        data[i] = 10 * log10(data[i]);
    }
}

auto STFT(const std::vector<double>& audioData)
{
    //fft设置
	constexpr int WINDOW_SIZE = 2048;
	constexpr int HOP_SIZE = WINDOW_SIZE / 4;
    constexpr int FFT_SIZE = WINDOW_SIZE / 2 + 1;
    const int NUM_FRAMES = (int(audioData.size()) - WINDOW_SIZE) / HOP_SIZE + 1;
	std::vector hannWindow(WINDOW_SIZE, 0.0);
	const auto fftIn = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * WINDOW_SIZE));
	const auto fftOut = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * FFT_SIZE));
    const fftw_plan plan = fftw_plan_dft_r2c_1d(WINDOW_SIZE, hannWindow.data(), fftOut, FFTW_ESTIMATE);
    std::vector spectrogram(NUM_FRAMES, std::vector<double>(FFT_SIZE));
    for (int i = 0; i < NUM_FRAMES; i++) {
        //应用汉宁窗
        std::memcpy(hannWindow.data(), &audioData[i * HOP_SIZE], size_t(sizeof(double)) * WINDOW_SIZE);
        HannWindow(hannWindow.data(), WINDOW_SIZE);
        //fft
        ConvertDoubleToFloat(std::vector(hannWindow.data(), hannWindow.data() + WINDOW_SIZE), reinterpret_cast<float*>(fftIn));
        fftw_execute(plan);
    	const auto real = reinterpret_cast<double*>(fftOut);
    	const double* imag = real + 1;
        CalculatePowerSpectrum(real, imag, FFT_SIZE);
        ConvertPowerSpectrumToDecibels(real, FFT_SIZE);
        for (int j = 0; j < FFT_SIZE; j++)
            spectrogram[i][j] = real[j];
    }
    fftw_free(fftIn);
    fftw_free(fftOut);
    fftw_destroy_plan(plan);
    return spectrogram;
}

spectrogramWidget::spectrogramWidget(QWidget* parent) :
	QWidget(parent)
{
	imageSpec.fill(backgroundColor);
}

spectrogramWidget::~spectrogramWidget() = default;

void spectrogramWidget::setWavFile(const std::wstring& path)
{
	std::vector<std::vector<double>> m_SpecData;
	try
	{
		auto I_Wavefile = AudioPreprocess().codec(path, 32000);
		if (I_Wavefile.size() < 16000)
			I_Wavefile.insert(I_Wavefile.end(), 16000 - I_Wavefile.size(), 0);
		std::vector D_WavFile(I_Wavefile.size(), 0.0);
		for (size_t i = 0; i < I_Wavefile.size(); ++i)
			D_WavFile[i] = static_cast<double>(I_Wavefile[i]) / 32768.0;
		m_SpecData = STFT(D_WavFile);
	}
	catch (std::exception& e)
	{
		m_SpecData.clear();
	}
	getImage(m_SpecData);
	update();
}

void spectrogramWidget::paintEvent(QPaintEvent* e)
{
	drawSpecData();
}

void spectrogramWidget::getImage(std::vector<std::vector<double>> m_SpecData)
{
	imageSpec.fill(backgroundColor);
	QPainter p(&imageSpec);
	if (m_SpecData.empty())
		return;
	const int NUM_FRAMES = int(m_SpecData.size());
	const int FFT_SIZE = int(m_SpecData[0].size());
	const double binWidth = static_cast<double>(32000) / FFT_SIZE;
	const double xScale = 1024 / static_cast<double>(NUM_FRAMES);
	constexpr auto s = 32000;
	constexpr double yScale = double(768) / s;
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		for (int j = 0; j < FFT_SIZE; j++)
		{
			//根据响度映射颜色
			constexpr double MAX_DB = 0.0;
			constexpr double MIN_DB = -80.0;
			double db = m_SpecData[i][j];
			if (db > MAX_DB)
				db = MAX_DB;
			else if (db < MIN_DB)
				db = MIN_DB;
			const double frac = (db - MIN_DB) / (MAX_DB - MIN_DB);
			const int colorIndex = static_cast<int>(frac * double(colorMap.size() - 1));
			auto color = colorMap[colorIndex];
			//绘制矩形
			const double x = i * xScale;
			const double y = (FFT_SIZE - j - 1) * binWidth * yScale;
			QRectF rect1 = QRectF(x, y, x + xScale, y + binWidth * yScale);
			p.setPen(QPen(color));
			p.drawRect(rect1);
		}
	}
}

void spectrogramWidget::drawSpecData()
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.drawImage(rect(), imageSpec);
}