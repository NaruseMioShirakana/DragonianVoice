#pragma once
#include "ModelBase.hpp"

INFERCLASSHEADER

class DiffusionSvc : public SVC
{
public:
    DiffusionSvc(const MJson&, const callback&, const callback_params&, Device _dev = Device::CPU);

	~DiffusionSvc() override;

    std::vector<int16_t> InferWithF0AndHiddenUnit(std::vector<MoeVSProject::Params>&) const;

    bool IsV2() const
    {
        return V2;
    }

    [[nodiscard]] std::vector<int16_t> InferCurAudio(MoeVSProject::Params& input_audio_infer) override;

    std::vector<int16_t> Inference(std::wstring& _inputLens) const override;
private:
    Ort::Session* diffSvc = nullptr;
    Ort::Session* nsfHifigan = nullptr;
    Ort::Session* encoder = nullptr;
    Ort::Session* denoise = nullptr;
    Ort::Session* pred = nullptr;
    Ort::Session* after = nullptr;

    const std::vector<const char*> nsfInput = { "c", "f0" };
    const std::vector<const char*> nsfOutput = { "audio" };
    const std::vector<const char*> DiffInput = { "hubert", "mel2ph", "spk_embed", "f0", "initial_noise", "speedup" };
    const std::vector<const char*> DiffOutput = { "mel_pred", "f0_pred" };
    const std::vector<const char*> encoderInput = { "hubert", "mel2ph", "spk_embed", "f0" };
    const std::vector<const char*> encoderInputddsp = { "hubert", "mel2ph", "f0", "volume", "spk_mix" };
    const std::vector<const char*> encoderInputSpkMix = { "hubert", "mel2ph", "f0", "spk_mix" };
    const std::vector<const char*> denoiseInput = { "noise", "time", "condition" };
    const std::vector<const char*> predInput = { "noise", "noise_pred", "time", "time_prev" };
    const std::vector<const char*> afterInput = { "x" };
    const std::vector<const char*> encoderOutput = { "mel_pred", "f0_pred" };
    const std::vector<const char*> encoderOutputDDSP = { "mel_pred" };
    const std::vector<const char*> denoiseOutput = { "noise_pred" };
    const std::vector<const char*> predOutput = { "noise_pred_o" };
    const std::vector<const char*> afterOutput = { "mel_out" };
};

INFERCLASSEND