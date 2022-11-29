#pragma once
#ifndef SHIRAKANAMIOTENSOR
#define SHIRAKANAMIOTENSOR
#include <onnxruntime_cxx_api.h>
#include <cmath>
#include <fstream>
typedef int64_t int64;
typedef Ort::Value MTensor;
constexpr float gateThreshold = 0.666f;
constexpr int64 maxDecoderSteps = 3000i64;
namespace STensor {
    /// <summary>
    /// 获取一个初始值为0的指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">张量内部数据类型</typeparam>
    /// <param name="shape">张量的shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>初始值为0，指定shape的张量</returns>
    template <typename __ty>
    MTensor getZero(std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs);

    template <typename __ty>
    MTensor getNumTensor(__ty num, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo);

    template <typename __ty>
    MTensor squeezeCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo);

    template <typename __ty>
    MTensor squeezeNoCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo);

    template <typename __ty>
    MTensor leCpp(MTensor&& inputTensor, __ty Other, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs);

    template <typename __ty>
    MTensor fromArray(__ty* inputArray, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo);

    template <typename __ty>
    MTensor sigmoid(MTensor& input, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs);

    template <typename __ty>
    MTensor unsqueezeCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo, std::vector<void*> &_ptrs);

    template <typename __ty>
    MTensor unsqueezeNoCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo);

    inline MTensor cats(MTensor& tensorA, MTensor&& tensorB, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrVec) {
        std::vector<int64> Shape = tensorA.GetTensorTypeAndShapeInfo().GetShape();
        Shape[2]++;
        int64 tensorSize = Shape[0] * Shape[1] * Shape[2];
        float* data = (float*)malloc(sizeof(float) * tensorSize);
        if (data == NULL) throw std::exception("内存不足");
        for (int64 i = 0; i < Shape[1]; i++) {
            memcpy(data + (i * Shape[2]), tensorA.GetTensorData<float>() + (i * (Shape[2] - 1)), (Shape[2] - 1) * sizeof(float));
            *(data + ((i * Shape[2]) + Shape[2] - 1)) = tensorB.GetTensorData<float>()[i];
        }
        _ptrVec.push_back(data);
        return Ort::Value::CreateTensor<float>(memoryInfo, data, tensorSize, Shape.data(), Shape.size());
    }

    template <typename __ty>
    MTensor copy(MTensor& tensorA, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrVec)
    {
        std::vector<int64> Shape = tensorA.GetTensorTypeAndShapeInfo().GetShape();
        int64 size = 1;
        for (const auto it : Shape)
            size *= it;
        __ty* data = (__ty*)malloc(sizeof(__ty) * size);
        if (data == nullptr)
            throw std::exception("OOM");
        memcpy(data, tensorA.GetTensorMutableData<__ty>(), sizeof(__ty) * size);
        _ptrVec.push_back(data);
        return Ort::Value::CreateTensor<float>(memoryInfo, data, size, Shape.data(), Shape.size());
    }
}

template <typename __ty>
MTensor STensor::getZero(std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    memset(data, static_cast<__ty>(0), sizeof(__ty) * tensorSize);
    auto _ret = MTensor::CreateTensor(memoryInfo, data, tensorSize, shape.data(), shape.size());
    _ptrs.push_back(data);
    return _ret;
}
template <typename __ty>
MTensor STensor::getNumTensor(__ty num, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(data + i) = num;
    return MTensor::CreateTensor(memoryInfo, data, tensorSize, shape.data(), shape.size());
}

template <typename __ty>
MTensor STensor::squeezeCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape;
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    for (int64 i : inputTensor.GetTensorTypeAndShapeInfo().GetShape())
        if (i - 1)shape.push_back(i);
    __ty* newTensorData = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (newTensorData == NULL) throw std::exception("内存不足");
    memcpy(newTensorData, inputTensor.GetTensorData<__ty>(), sizeof(__ty) * tensorSize);
    return MTensor::CreateTensor<__ty>(
        memoryInfo, newTensorData, tensorSize, shape.data(), shape.size());
}

template <typename __ty>
MTensor STensor::squeezeNoCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape;
    for (int64 i : inputTensor.GetTensorTypeAndShapeInfo().GetShape())
        if (i - 1)shape.push_back(i);
    return MTensor::CreateTensor<__ty>(
        memoryInfo, inputTensor.GetTensorMutableData<__ty>(), inputTensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}

template <typename __ty>
MTensor STensor::leCpp(MTensor&& inputTensor, __ty Other, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs) {
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other;
    auto _out = MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
    _ptrs.push_back(boolTensorData);
    return _out;
}

template <typename __ty>
MTensor STensor::fromArray(__ty* inputArray, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    return MTensor::CreateTensor<__ty>(memoryInfo, inputArray, tensorSize, shape.data(), shape.size());
}

template <typename __ty>
MTensor STensor::sigmoid(MTensor& input, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs) {
    int64 tensorSize = input.GetTensorTypeAndShapeInfo().GetElementCount();
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(data+i)= (__ty)(1.0F / (1.0F + exp(0.0F - input.GetTensorData<__ty>()[i])));
    _ptrs.push_back(data);
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, input.GetTensorTypeAndShapeInfo().GetShape().data(), input.GetTensorTypeAndShapeInfo().GetShape().size());
}

template <typename __ty>
MTensor STensor::unsqueezeCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs) {
    std::vector<int64> shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (D > shape.size()) throw std::exception("下标越界");
    for (int i = 0; i < C; i++)
        shape.insert(shape.begin() + D, 1);
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensor.GetTensorTypeAndShapeInfo().GetElementCount());
    if (data == NULL) throw std::exception("内存不足");
    memcpy(data, tensor.GetTensorData<__ty>(), sizeof(__ty) * tensor.GetTensorTypeAndShapeInfo().GetElementCount());
    auto _out = MTensor::CreateTensor(memoryInfo, data, tensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
    _ptrs.push_back(data);
    return _out;
}

template <typename __ty>
MTensor STensor::unsqueezeNoCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (D > shape.size()) throw std::exception("下标越界");
    for (int i = 0; i < C; i++)
        shape.insert(shape.begin() + D, 1);
    return MTensor::CreateTensor(memoryInfo, tensor.GetTensorMutableData<__ty>(), tensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}

const std::vector<const char*> ganIn = { "x" };
const std::vector<const char*> ganOut = { "audio" };
const std::vector<const char*> inputNodeNamesSessionEncoder = { "sequences","sequence_lengths" };
const std::vector<const char*> outputNodeNamesSessionEncoder = { "memory","processed_memory","lens" };
const std::vector<const char*> inputNodeNamesSessionDecoderIter = { "decoder_input","attention_hidden","attention_cell","decoder_hidden","decoder_cell","attention_weights","attention_weights_cum","attention_context","memory","processed_memory","mask" };
const std::vector<const char*> outputNodeNamesSessionDecoderIter = { "decoder_output","gate_prediction","out_attention_hidden","out_attention_cell","out_decoder_hidden","out_decoder_cell","out_attention_weights","out_attention_weights_cum","out_attention_context" };
const std::vector<const char*> inputNodeNamesSessionPostNet = { "mel_outputs" };
const std::vector<const char*> outputNodeNamesSessionPostNet = { "mel_outputs_postnet" };
const std::vector<const char*> DecInput = { "z_in", "g" };
const std::vector<const char*> DecOutput = { "o" };
const std::vector<const char*> DpInput = { "x", "x_mask", "g" };
const std::vector<const char*> DpOutput = { "logw" };
const std::vector<const char*> EncInput = { "x", "x_lengths" };
const std::vector<const char*> EncOutput = { "xout", "m_p", "logs_p", "x_mask" };
const std::vector<const char*> FlowInput = { "z_p", "y_mask", "g" };
const std::vector<const char*> FlowOutput = { "z" };
const std::vector<const char*> EMBInput = { "sid" };
const std::vector<const char*> EMBOutput = { "g" };

std::vector<MTensor> initDecoderInputs(std::vector<MTensor>& inputTens, Ort::MemoryInfo& memoryInfo, std::vector<void*>& _ptrs) {
    std::vector<int64> shape1 = inputTens[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> shape2 = inputTens[1].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> attention_rnn_dim{ shape1[0],1024i64 };
    std::vector<int64> decoder_rnn_dim{ shape1[0],1024i64 };
    std::vector<int64> encoder_embedding_dim{ shape1[0],512i64 };
    std::vector<int64> n_mel_channels{ shape1[0],80i64 };
    std::vector<int64> seqLen{ shape1[0],shape1[1] };
    std::vector<MTensor> outTensorVec;
    bool* tempBoolean = static_cast<bool*>(malloc(sizeof(bool) * seqLen[0] * seqLen[1]));
    if (tempBoolean == NULL) throw std::exception("Null");
    memset(tempBoolean, 0, seqLen[0] * seqLen[1]);
    try {
        outTensorVec.push_back(STensor::getZero<float>(n_mel_channels, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(attention_rnn_dim, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(attention_rnn_dim, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(decoder_rnn_dim, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(decoder_rnn_dim, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(seqLen, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(seqLen, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::getZero<float>(encoder_embedding_dim, memoryInfo, _ptrs));
        outTensorVec.push_back(STensor::fromArray(inputTens[0].GetTensorMutableData<float>(), shape1, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(inputTens[1].GetTensorMutableData<float>(), shape2, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(tempBoolean, seqLen, memoryInfo));
    }
    catch (Ort::Exception e) {
        outTensorVec.clear();
        return outTensorVec;
    }
    _ptrs.push_back(tempBoolean);
    return outTensorVec;
}
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
#endif