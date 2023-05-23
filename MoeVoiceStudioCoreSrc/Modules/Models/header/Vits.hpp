#pragma once
#include "ModelBase.hpp"

INFERCLASSHEADER

class Vits : public TTS
{
public:
    Vits(const rapidjson::Document&, const callback&, const callback_params&, const DurationCallback&, Device _dev = Device::CPU);

	~Vits() override;

    std::vector<int16_t> Inference(std::wstring& _inputLens) const override;
private:
    Ort::Session* sessionDec = nullptr;
    Ort::Session* sessionDp = nullptr;
    Ort::Session* sessionEnc_p = nullptr;
    Ort::Session* sessionFlow = nullptr;
    Ort::Session* sessionEmb = nullptr;

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