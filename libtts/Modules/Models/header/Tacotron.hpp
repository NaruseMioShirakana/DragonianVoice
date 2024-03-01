#pragma once
#include "ModelBase.hpp"

INFERCLASSHEADER

class Tacotron2 : public TTS
{
public:
    Tacotron2(const MJson&, const callback&, const callback_params&, const DurationCallback&, Device _dev = Device::CPU);

	~Tacotron2() override;

    std::vector<int16_t> Inference(std::wstring& _inputLens) const override;

    [[nodiscard]] std::vector<int16_t> Inference(const MoeVSProject::TTSParams& _input) const override;

    static void cat(std::vector<float>& tensorA, std::vector<int64>& Shape, const MTensor& tensorB) {
        const int64 n = Shape[1];
        for (int64 i = n; i > 0; --i)
            tensorA.insert(tensorA.begin() + (i * Shape[2]), tensorB.GetTensorData<float>()[i - 1]);
        ++Shape[2];
    }
private:
    Ort::Session* sessionEncoder = nullptr;
    Ort::Session* sessionDecoderIter = nullptr;
    Ort::Session* sessionPostNet = nullptr;
    Ort::Session* sessionGan = nullptr;

    const std::vector<const char*> ganIn = { "x" };
    const std::vector<const char*> ganOut = { "audio" };
    const std::vector<const char*> inputNodeNamesSessionEncoder = { "sequences","sequence_lengths" };
    const std::vector<const char*> outputNodeNamesSessionEncoder = { "memory","processed_memory","lens" };
    const std::vector<const char*> inputNodeNamesSessionDecoderIter = { "decoder_input","attention_hidden","attention_cell","decoder_hidden","decoder_cell","attention_weights","attention_weights_cum","attention_context","memory","processed_memory","mask" };
    const std::vector<const char*> outputNodeNamesSessionDecoderIter = { "decoder_output","gate_prediction","out_attention_hidden","out_attention_cell","out_decoder_hidden","out_decoder_cell","out_attention_weights","out_attention_weights_cum","out_attention_context" };
    const std::vector<const char*> inputNodeNamesSessionPostNet = { "mel_outputs" };
    const std::vector<const char*> outputNodeNamesSessionPostNet = { "mel_outputs_postnet" };
};

INFERCLASSEND