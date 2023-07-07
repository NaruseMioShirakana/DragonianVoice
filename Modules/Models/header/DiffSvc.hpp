#pragma once
#include "SVC.hpp"

MoeVoiceStudioCoreHeader

class DiffusionSvc : public SingingVoiceConversion
{
public:
    DiffusionSvc(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

	~DiffusionSvc() override;

    [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice,
        const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

    [[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Paths, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
        const InferTools::SlicerSettings& _SlicerSettings) const override;

    [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSliceRef& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

private:
    Ort::Session* diffSvc = nullptr;
    Ort::Session* nsfHifigan = nullptr;
    Ort::Session* naive = nullptr;

    Ort::Session* encoder = nullptr;
    Ort::Session* denoise = nullptr;
    Ort::Session* pred = nullptr;
    Ort::Session* alpha = nullptr;
    Ort::Session* after = nullptr;

    int64_t melBins = 128;
    int64_t Pndms = 100;
    int64_t MaxStep = 1000;

    std::wstring DiffSvcVersion = L"DiffSvc";

    const std::vector<const char*> nsfInput = { "c", "f0" };
    const std::vector<const char*> nsfOutput = { "audio" };
    const std::vector<const char*> DiffInput = { "hubert", "mel2ph", "spk_embed", "f0", "initial_noise", "speedup" };
    const std::vector<const char*> DiffOutput = { "mel_pred", "f0_pred" };
    const std::vector<const char*> afterInput = { "x" };
    const std::vector<const char*> afterOutput = { "mel_out" };
    const std::vector<const char*> naiveOutput = { "mel" };
};

MoeVoiceStudioCoreEnd