#pragma once
#include "ModelBase.hpp"

INFERCLASSHEADER

class Vits : public BaseModelType
{
public:
    using DurationCallback = std::function<void(const std::vector<float>&)>;

    Vits(const rapidjson::Document&, const callback&, const callback_params&, const DurationCallback&);

    virtual ~Vits();

    std::vector<int16_t> Infer(std::wstring&) override;

    static std::vector<std::vector<bool>> generatePath(float* duration, size_t durationSize, size_t maskSize);

    std::vector<float> GetEmotionVector(std::wstring src)
    {
        std::vector<float> dst(1024, 0.0);
        std::wsmatch mat;
        uint64_t mul = 0;
        while (std::regex_search(src, mat, EmoReg))
        {
            long emoId;
            const auto emoStr = to_byte_string(mat.str());
            if (!EmoJson[emoStr.c_str()].Empty())
                emoId = EmoJson[emoStr.c_str()].GetInt();
            else
                emoId = atoi(emoStr.c_str());
            auto emoVec = emoLoader[emoId];
            for (size_t i = 0; i < 1024; ++i)
                dst[i] = dst[i] + (emoVec[i] - dst[i]) / (float)(mul + 1ull);
            src = mat.suffix();
            ++mul;
        }
        return dst;
    }
private:
    Ort::Session* sessionDec = nullptr;
    Ort::Session* sessionDp = nullptr;
    Ort::Session* sessionEnc_p = nullptr;
    Ort::Session* sessionFlow = nullptr;
    Ort::Session* sessionEmb = nullptr;
    long _samplingRate = 22050;
    std::map<std::wstring, int64_t> _Phs;
    std::map<wchar_t, int64_t> _Symbols;
    bool add_blank = true;
    bool use_ph = false;
    bool emo = false;
    EmoLoader emoLoader;
    std::string emoStringa;
    rapidjson::Document EmoJson;
    DurationCallback _dcb;

    const std::vector<const char*> DecInputTmp = { "z_in", "g" };
    const std::vector<const char*> DecOutput = { "o" };
    const std::vector<const char*> DpInputTmp = { "x", "x_mask", "zin", "g" };
    const std::vector<const char*> DpOutput = { "logw" };
    const std::vector<const char*> EncInput = { "x", "x_lengths" };
    const std::vector<const char*> EncPInput = { "x", "t", "x_lengths" };
    const std::vector<const char*> EnceInput = { "x", "x_lengths", "emotion" };
    const std::vector<const char*> EncOutput = { "xout", "m_p", "logs_p", "x_mask" };
    const std::vector<const char*> FlowInputTmp = { "z_p", "y_mask", "g" };
    const std::vector<const char*> FlowOutput = { "z" };
    const std::vector<const char*> EMBInput = { "sid" };
    const std::vector<const char*> EMBOutput = { "g" };
};

INFERCLASSEND