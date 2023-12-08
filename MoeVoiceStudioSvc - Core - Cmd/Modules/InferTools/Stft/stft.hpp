#pragma once
#include <vector>
#include "fftw3.h"

namespace DlCodecStft
{
    class STFT
    {
    public:
        STFT() = default;
        ~STFT();
        STFT(int WindowSize, int HopSize, int FFTSize = 0);
        inline static double PI = 3.14159265358979323846;
        std::pair<std::vector<float>, int64_t> operator()(const std::vector<double>& audioData) const;
    private:
    	int WINDOW_SIZE = 2048;
    	int HOP_SIZE = WINDOW_SIZE / 4;
    	int FFT_SIZE = WINDOW_SIZE / 2 + 1;
    };

    class Mel
    {
    public:
        Mel() = delete;
        ~Mel() = default;
        Mel(int WindowSize, int HopSize, int SamplingRate, int MelSize = 0);
        std::pair<std::vector<float>, int64_t> GetMel(const std::vector<double>& audioData) const;
        std::pair<std::vector<float>, int64_t> operator()(const std::vector<double>& audioData) const;
    private:
        STFT stft;
        int MEL_SIZE = 128;
        int FFT_SIZE = 0;
        int sr = 22050;
        std::vector<float> MelBasis;
    };
}
