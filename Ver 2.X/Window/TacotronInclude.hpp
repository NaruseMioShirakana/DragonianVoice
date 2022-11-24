#pragma once
#ifndef __TACOTRON2INCLUDE__
#define __TACOTRON2INCLUDE__
#include <D:\VisualStudioProj\Tacotron inside\MioShirakanaTensor.hpp>
#include <locale>
#include <codecvt>
#include <map>
#include <direct.h>
#include <fstream>
#include <thread>
constexpr float gateThreshold = 0.666f;
constexpr int64 maxDecoderSteps = 5000i64;
typedef int32_t int32;
struct WavHead {
    char RIFF[4];
    long int size0;
    char WAVE[4];
    char FMT[4];
    long int size1;
    short int fmttag;
    short int channel;
    long int samplespersec;
    long int bytepersec;
    short int blockalign;
    short int bitpersamples;
    char DATA[4];
    long int size2;
};
int conArr2Wav(int64 size, int16_t* input, const char* filename) {
    WavHead head = { {'R','I','F','F'},0,{'W','A','V','E'},{'f','m','t',' '},16,
            1,1,22050,22050 * 2,2,16,{'d','a','t','a'},
            0 };
    const long Size = static_cast<long>(size);
    head.size0 = Size * 2l + 36l;
    head.size2 = Size * 2l;
    std::ofstream Out;
    const char* outputData = (char*)(input);
    Out.open(filename, std::ios::out | std::ios::binary);
    Out.write((char*)(&head), 44);
    Out.write(outputData, static_cast<std::streamsize>(size) * 2ll);
    Out.close();
    return 0;
}
inline std::wstring to_wide_string(const std::string& input)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}
inline std::string to_byte_string(const std::wstring& input)
{
    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(input);
}
const std::vector<const char*> ganIn = { "x" };
const std::vector<const char*> ganOut = { "audio" };
const std::vector<const char*> inputNodeNamesSessionEncoder = { "sequences","sequence_lengths" };
const std::vector<const char*> outputNodeNamesSessionEncoder = { "memory","processed_memory","lens" };
const std::vector<const char*> inputNodeNamesSessionDecoderIter = { "decoder_input","attention_hidden","attention_cell","decoder_hidden","decoder_cell","attention_weights","attention_weights_cum","attention_context","memory","processed_memory","mask" };
const std::vector<const char*> outputNodeNamesSessionDecoderIter = { "decoder_output","gate_prediction","out_attention_hidden","out_attention_cell","out_decoder_hidden","out_decoder_cell","out_attention_weights","out_attention_weights_cum","out_attention_context" };
const std::vector<const char*> inputNodeNamesSessionPostNet = { "mel_outputs" };
const std::vector<const char*> outputNodeNamesSessionPostNet = { "mel_outputs_postnet" };
std::vector<MTensor> initDecoderInputs(std::vector<MTensor>& inputTens, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape1 = inputTens[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> shape2 = inputTens[1].GetTensorTypeAndShapeInfo().GetShape();
    std::vector attention_rnn_dim{ shape1[0],1024i64 };
    std::vector decoder_rnn_dim{ shape1[0],1024i64 };
    std::vector encoder_embedding_dim{ shape1[0],512i64 };
    std::vector n_mel_channels{ shape1[0],80i64 };
    std::vector seqLen{ shape1[0],shape1[1] };
    std::vector<MTensor> outTensorVec;
    bool* tempBoolean = static_cast<bool*>(malloc(sizeof(bool) * seqLen[0] * seqLen[1]));
    if (tempBoolean == NULL) throw std::exception("Null");
    memset(tempBoolean, 0, seqLen[0] * seqLen[1]);
    try {
        outTensorVec.push_back(STensor::getZero<float>(n_mel_channels, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(attention_rnn_dim, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(attention_rnn_dim, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(decoder_rnn_dim, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(decoder_rnn_dim, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(seqLen, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(seqLen, memoryInfo));
        outTensorVec.push_back(STensor::getZero<float>(encoder_embedding_dim, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(inputTens[0].GetTensorMutableData<float>(), shape1, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(inputTens[1].GetTensorMutableData<float>(), shape2, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(tempBoolean, seqLen, memoryInfo));
    }
    catch (Ort::Exception e) {
        outTensorVec.clear();
        return outTensorVec;
    }
    return outTensorVec;
}
#endif