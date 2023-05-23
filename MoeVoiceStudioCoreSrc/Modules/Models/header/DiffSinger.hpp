#pragma once
#include "ModelBase.hpp"

INFERCLASSHEADER

class DiffusionSinger : public BaseModelType
{
public:
    struct DiffSingerInput
    {
        std::vector<std::vector<int64_t>> inputLens;
        std::vector<std::vector<int64_t>> durations;
        std::vector<std::vector<bool>> is_slur;
        std::vector<std::vector<int64_t>> pitch_durations;
        std::vector<std::vector<int64_t>> pitchs;
        std::vector<std::vector<float>> f0;
        std::vector<double> offset;
    };

    DiffusionSinger(const rapidjson::Document&, const callback&, const callback_params&, Device _dev = Device::CPU);

	~DiffusionSinger() override;

    std::vector<int16_t> Inference(std::wstring& _inputLens) const override;

    static std::map<std::wstring, std::vector<std::wstring>> GetPhonesPairMap(const std::wstring& path);

    static std::map<std::wstring, int64_t> GetPhones(const std::map<std::wstring, std::vector<std::wstring>>& PhonesPair);

    std::vector<DiffSingerInput> preprocessDiffSinger(const std::vector<std::wstring>& Jsonpath) const;

    static std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0)
    {
        std::vector<double> output;
        while (start < end)
        {
            output.push_back(start / div);
            start += step;
        }
        return output;
    }

    static std::vector<double> linspace(size_t step, double start = 0.0, double end = 1.0)
    {
        const double off = (end - start) / static_cast<double>(step);
        std::vector<double> out(step + 1);
        for (size_t i = 0; i <= step; ++i)
            out[i] = static_cast<double>(i) * off + start;
        return out;
    }
private:
    //Diffusion
    Ort::Session* nsfHifigan = nullptr;
    Ort::Session* pred = nullptr;
    Ort::Session* denoise = nullptr;
    Ort::Session* after = nullptr;
    Ort::Session* encoder = nullptr;

    //Singer
    Ort::Session* diffSinger = nullptr;

    int hop = 320;
    long melBins = 256;
    int64_t Hidden_Size = 256;
    bool MidiVer = false;

    std::map<std::wstring, std::vector<std::wstring>> PhonesPair;
    std::map<std::wstring, int64_t> Phones;

    const std::vector<const char*> encoderInput = { "hubert", "mel2ph", "spk_embed", "f0" };
    const std::vector<const char*> denoiseInput = { "noise", "time", "condition" };
    const std::vector<const char*> predInput = { "noise", "noise_pred", "time", "time_prev" };
    const std::vector<const char*> afterInput = { "x" };
    const std::vector<const char*> encoderOutput = { "mel_pred", "f0_pred" };
    const std::vector<const char*> denoiseOutput = { "noise_pred" };
    const std::vector<const char*> predOutput = { "noise_pred_o" };
    const std::vector<const char*> afterOutput = { "mel_out" };

    const std::vector<const char*> SingerEncoderInput = { "tokens", "durations", "f0", "spk_embed" };
    const std::vector<const char*> SingerEncoderOutput = { "condition" };
    const std::vector<const char*> SingerHifiganInput = { "mel", "f0" };
    const std::vector<const char*> SingerHifiganOutput = { "waveform" };

    const std::vector<const char*> SingerInput = { "tokens", "durations", "f0", "speedup", "spk_embed" };
    const std::vector<const char*> SingerOutput = { "mel" };

    std::wregex tokenReg = std::wregex(L"[A-Za-z]+");
    std::regex pitchReg = std::regex("[A-Za-z0-9#/]+");
    std::regex noteReg = std::regex("([A-Za-z#]+)([0-9]+)");
    std::regex numReg = std::regex("[0-9\\.]+");
    std::map<std::string, int64_t> midiPitch{
        {"C",0},{"C#",1},{"D",2},{"D#",3},{"E",4},{"F",5},
        {"F#",6},{"G",7},{"G#",8},{"A",9},{"A#",10},{"B",11},
        {"Db",1},{"Eb",3},{"Gb",6},{"Ab",8},{"Bb",10}
    };
};

INFERCLASSEND