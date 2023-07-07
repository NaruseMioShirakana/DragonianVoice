#pragma once
#include "SVC.hpp"
#ifdef WIN32
#ifdef MoeVSMui
#include <queue>
#include <Render/Sound/Mui_DirectSound.h>
#include "../../AvCodec/Recorder.h"
#endif
#endif

MoeVoiceStudioCoreHeader

class VitsSvc : public SingingVoiceConversion
{
public:
    VitsSvc(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

	~VitsSvc() override;

    [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice,
        const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

	[[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Paths, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
        const InferTools::SlicerSettings& _SlicerSettings) const override;

    //[[nodiscard]] std::vector<Ort::Value> InferSliceTensor(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice, size_t SliceIdx,
    //    const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, std::vector<Ort::Value>& _Tensors,
    //    std::vector<const char*>& SoVitsInput) const;

    [[nodiscard]] std::vector<int16_t> InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

#ifdef WIN32
#ifdef MoeVSMui
    void StartRT(Mui::Window::UIWindowBasic* window, const MoeVSProjectSpace::MoeVSSvcParams& _Params);
    void EndRT();
#endif
#endif
private:
    Ort::Session* VitsSvcModel = nullptr;

    std::wstring VitsSvcVersion = L"SoVits4.0";

    const std::vector<const char*> soVitsOutput = { "audio" };
    const std::vector<const char*> soVitsInput = { "hidden_unit", "lengths", "pitch", "sid" };
    const std::vector<const char*> RVCInput = { "phone", "phone_lengths", "pitch", "pitchf", "ds", "rnd" };
#ifdef WIN32
#ifdef MoeVSMui
    bool RTSTAT = false;
    std::deque<std::vector<int16_t>> inputBuffer, outputBuffer, rawInputBuffer, rawOutputBuffer;
    MRecorder* recoder = nullptr;
    Mui::Render::MDS_AudioPlayer* audio_player = nullptr;
    Mui::Render::MAudioStream* audio_stream = nullptr;
    size_t emptyLen = 30000;
#endif
#endif
};

MoeVoiceStudioCoreEnd
