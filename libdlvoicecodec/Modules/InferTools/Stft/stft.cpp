#include "stft.hpp"
#include "../inferTools.hpp"
#include "../../Logger/MoeSSLogger.hpp"
#include "cblas.h"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace DlCodecStft
{
    void HannWindow(double* data, int size) {
        for (int i = 0; i < size; i++) {
            const double windowValue = 0.5 * (1 - cos(2 * STFT::PI * i / (size - 1)));
            data[i] *= windowValue;
        }
    }

    void ConvertDoubleToFloat(const std::vector<double>& input, float* output)
    {
        for (size_t i = 0; i < input.size(); i++) {
            output[i] = static_cast<float>(input[i]);
        }
    }

    double CalculatePowerSpectrum(fftw_complex fc) {
        return sqrt(fc[0] * fc[0] + fc[1] * fc[1]);
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

	double HZ2Mel(const double frequency)
    {
        constexpr auto f_min = 0.0;
        constexpr auto f_sp = 200.0 / 3;
        auto mel = (frequency - f_min) / f_sp;
        constexpr auto min_log_hz = 1000.0;
        constexpr auto min_log_mel = (min_log_hz - f_min) / f_sp;
        const auto logstep = log(6.4) / 27.0;
        if (frequency >= min_log_hz)
            mel = min_log_mel + log(frequency / min_log_hz) / logstep;
        return mel;
    }

    double Mel2HZ(const double mel)
    {
        constexpr auto f_min = 0.0;
        constexpr auto f_sp = 200.0 / 3;
        auto freqs = f_min + f_sp * mel;
        constexpr auto min_log_hz = 1000.0;
        constexpr auto min_log_mel = (min_log_hz - f_min) / f_sp;
        const auto logstep = log(6.4) / 27.0;
        if (mel >= min_log_mel)
            freqs = min_log_hz * exp(logstep * (mel - min_log_mel));
        return freqs;
    }

    static double mel_min = HZ2Mel(20.);
    static double mel_max = HZ2Mel(11025.);

    STFT::STFT(int WindowSize, int HopSize, int FFTSize)
    {
        WINDOW_SIZE = WindowSize;
        HOP_SIZE = HopSize;
        if (FFTSize > 0)
            FFT_SIZE = FFTSize;
        else
            FFT_SIZE = WINDOW_SIZE / 2 + 1;
    }

    STFT::~STFT() = default;

    std::pair<std::vector<float>, int64_t> STFT::operator()(const std::vector<double>& audioData) const
    {
        const int NUM_FRAMES = (int(audioData.size()) - WINDOW_SIZE) / HOP_SIZE + 1;
        std::vector hannWindow(WINDOW_SIZE, 0.0);
        const auto fftOut = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * FFT_SIZE));
        const fftw_plan plan = fftw_plan_dft_r2c_1d(WINDOW_SIZE, hannWindow.data(), fftOut, FFTW_ESTIMATE);
        std::vector spectrogram(size_t(NUM_FRAMES) * FFT_SIZE, 0.f);
        for (int i = 0; i < NUM_FRAMES; i++) {
            std::memcpy(hannWindow.data(), &audioData[size_t(i) * HOP_SIZE], size_t(sizeof(double)) * WINDOW_SIZE);
            HannWindow(hannWindow.data(), WINDOW_SIZE);
            fftw_execute(plan);
            const auto BgnPtn = size_t(unsigned(i * FFT_SIZE));
            for (int j = 0; j < FFT_SIZE; j++)
                spectrogram[BgnPtn + j] = float(CalculatePowerSpectrum(fftOut[j]));
        }
        fftw_destroy_plan(plan);
        fftw_free(fftOut);
        return { std::move(spectrogram), int64_t(NUM_FRAMES) };
    }

    std::pair<std::vector<float>, int64_t> Mel::GetMel(const std::vector<double>& audioData) const
    {
        auto BgnTime = clock();
        const auto Spec = stft(audioData);  //[frame, nfft] * [nfft, mel_bins]  |  [mel_bins, nfft] * [nfft, frame]
        logger.log(("[Inference] Slice Stft Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
        const auto NFrames = Spec.second;
        std::vector Mel(MEL_SIZE * NFrames, 0.f);
        BgnTime = clock();
        cblas_sgemm(
            CblasRowMajor,
            CblasNoTrans,
            CblasTrans,
            MEL_SIZE,
            blasint(NFrames),
            FFT_SIZE,
            1.f,
            MelBasis.data(),
            FFT_SIZE,
            Spec.first.data(),
            blasint(FFT_SIZE),
            0.f,
            Mel.data(),
            blasint(NFrames)
        );
        for (auto& it : Mel)
            it = log(std::max(1e-5f, it));
        logger.log(("[Inference] Slice Mel Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
        return { std::move(Mel), (int64_t)NFrames };
    }

    std::pair<std::vector<float>, int64_t> Mel::operator()(const std::vector<double>& audioData) const
    {
        return GetMel(audioData);
    }

    Mel::Mel(int WindowSize, int HopSize, int SamplingRate, int MelSize) :
        stft(WindowSize, HopSize, WindowSize / 2 + 1)
    {
        if (MelSize > 0)
            MEL_SIZE = MelSize;
        FFT_SIZE = WindowSize / 2 + 1;
        sr = SamplingRate;

        const int nfft = (FFT_SIZE - 1) * 2;
        const double fftfreqval = 1. / (double(nfft) / double(SamplingRate));
        auto fftfreqs = InferTools::arange(0, FFT_SIZE + 2);
        fftfreqs.resize(FFT_SIZE);
        for (auto& i : fftfreqs)
            i *= fftfreqval;

        auto mel_f = InferTools::arange(mel_min, mel_max + 1., (mel_max - mel_min) / (MEL_SIZE + 1));
        mel_f.resize(MEL_SIZE + 2); //[MEL_SIZE + 2]

        std::vector<double> fdiff;
        std::vector<std::vector<double>> ramps; //[MEL_SIZE + 2, FFTSize]

        ramps.reserve(MEL_SIZE + 2);
        for (auto& i : mel_f)
        {
            i = Mel2HZ(i);
	        ramps.emplace_back(FFT_SIZE, i);
        }
        for (size_t i = 0; i < ramps.size(); ++i)
            for (int j = 0; j < FFT_SIZE; ++j)
                ramps[i][j] -= fftfreqs[j];

        fdiff.reserve(MEL_SIZE + 2); //[MEL_SIZE + 1]
        for (size_t i = 1; i < mel_f.size(); ++i)
            fdiff.emplace_back(mel_f[i] - mel_f[i - 1]);

        MelBasis = std::vector(size_t(FFT_SIZE) * MelSize, 0.f);

        for (int i = 0; i < MelSize; ++i)
        {
            const auto enorm = 2. / (mel_f[i + 2] - mel_f[i]);
            for (int j = 0; j < FFT_SIZE; ++j)
                MelBasis[i * FFT_SIZE + j] = (float)(std::max(0., std::min(-ramps[i][j] / fdiff[i], ramps[i + 2][j] / fdiff[i + 1])) * enorm);
        }
    }
}