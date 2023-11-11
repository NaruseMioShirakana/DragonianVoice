#pragma once
#include "TTS.hpp"

MoeVoiceStudioCoreHeader

class Vits : public TextToSpeech
{
public:
    Vits(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    Vits(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback, const std::vector<std::wstring>& _BertPaths,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    void load(const std::map<std::string, std::wstring>& _PathDict,
        const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback, const std::vector<std::wstring>& _BertPaths = {});

	~Vits() override;

    void destory()
    {
        delete sessionDec;
        delete sessionSdp;
        delete sessionDp;
        delete sessionEnc_p;
        delete sessionFlow;
        delete sessionEmb;
        sessionDec = nullptr;
        sessionSdp = nullptr;
        sessionEnc_p = nullptr;
        sessionFlow = nullptr;
        sessionEmb = nullptr;
        sessionDp = nullptr;
        for (auto& OrtPtr : sessionBert)
        {
	        delete OrtPtr;
            OrtPtr = nullptr;
        }
        sessionBert.clear();
    }

    [[nodiscard]] std::vector<std::vector<int16_t>> Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const override;
private:
    Ort::Session* sessionDec = nullptr;
    Ort::Session* sessionSdp = nullptr;
    Ort::Session* sessionDp = nullptr;
    Ort::Session* sessionEnc_p = nullptr;
    Ort::Session* sessionFlow = nullptr;
    Ort::Session* sessionEmb = nullptr;
    std::vector<Ort::Session*> sessionBert;
    std::vector<std::string> BertNames;
    std::string VitsType;
    bool UseTone = false;
    bool UseBert = false;
    bool UseLength = true;
    bool UseLanguage = false;
    bool EncoderG = false;

    std::vector<const char*> EncoderInputNames = { "x" };
    const std::vector<const char*> EncoderOutputNames = { "xout", "m_p", "logs_p", "x_mask" };

    std::vector<const char*> SdpInputNames = { "x", "x_mask", "zin" };
    const std::vector<const char*> SdpOutputNames = { "logw" };

    std::vector<const char*> DpInputNames = { "x", "x_mask" };
    const std::vector<const char*> DpOutputNames = { "logw" };

	std::vector<const char*> FlowInputNames = { "z_p", "y_mask" };
    const std::vector<const char*> FlowOutputNames = { "z" };

    std::vector<const char*> DecInputNames = { "z_in" };
    const std::vector<const char*> DecOutputNames = { "o" };

    const std::vector<const char*> EmbiddingInputNames = { "sid" };
    const std::vector<const char*> EmbiddingOutputNames = { "g" };

    const std::vector<const char*> BertInputNames = { "input_ids", "attention_mask", "token_type_ids" };
    const std::vector<const char*> BertOutputNames = { "last_hidden_state" };
};

MoeVoiceStudioCoreEnd