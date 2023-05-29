#pragma once
#include <queue>
#include "ModelBase.hpp"
#ifdef WIN32
#ifdef MoeVSMui
#include <Render/Sound/Mui_DirectSound.h>
#include "../../AvCodec/Recorder.h"
#endif
#endif

INFERCLASSHEADER
class VitsSvc : public SVC
{
public:
    VitsSvc(const rapidjson::Document&, const callback&, const callback_params&, Device _dev = Device::CPU);

	~VitsSvc() override;

    //ÒÑÆúÓÃ
    std::vector<int16_t> InferBatch() const;

    std::vector<int16_t> InferWithF0AndHiddenUnit(std::vector<MoeVSProject::Params>&) const;

	std::vector<int16_t> Inference(std::wstring& _inputLens) const override;

#ifdef WIN32
#ifdef MoeVSMui
    void StartRT(Mui::Window::UIWindowBasic* window) = delete;
    void EndRT();
    std::vector<int16_t> RTInference(const std::vector<int16_t>& PCMData, long srcSr) const;
    InferConfigs& GetRTParams()
    {
        return RTParams;
    }
#endif
#endif
private:
    Ort::Session* VitsSvcModel = nullptr;

    const std::vector<const char*> soVitsOutput = { "audio" };
    const std::vector<const char*> soVitsInput = { "hidden_unit", "lengths", "pitch", "sid" };
    const std::vector<const char*> RVCInput = { "phone", "phone_lengths", "pitch", "pitchf", "ds", "rnd" };

#ifdef WIN32
#ifdef MoeVSMui
    bool RTSTAT = false;
    std::deque<std::vector<int16_t>> inputBuffer, outputBuffer, rawInputBuffer;
    std::deque<std::pair<std::vector<int16_t>, size_t>> rawOutputBuffer;
    MRecorder* recoder = nullptr;
    Mui::MDS_AudioPlayer* audio_player = nullptr;
    Mui::MAudioStream* audio_stream = nullptr;
    InferConfigs RTParams;
#endif
#endif
};
INFERCLASSEND
