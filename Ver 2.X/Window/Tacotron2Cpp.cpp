#pragma warning(disable : 4996)

#include <D:/VisualStudioProj/Tacotron inside/TacotronInclude.hpp>

int main(int argc, char* argv[]) {
    if (argc != 5) return 0;
    wchar_t* buffer;
    if ((buffer = _wgetcwd(nullptr, 0)) == nullptr) return 0;
    std::wstring bufferStr = buffer;
    std::wstring PathTmp = to_wide_string(argv[1]);
    std::wstring Path = bufferStr + L"\\" + PathTmp;
    std::wstring TextInput = to_wide_string(argv[2]);
    std::wstring OutDir = to_wide_string(argv[3]);
    std::wstring HifiganPath = bufferStr + L"\\hifigan.onnx";
    std::wstring SymbolStr = to_wide_string(argv[4]);
    std::map<wchar_t, int64> Symbol;
    for (size_t i = 0; i < SymbolStr.length(); i++)
        Symbol.insert(std::pair<wchar_t, int64>(SymbolStr[i], (int64)(i)));
    auto text = static_cast<int64*>(malloc(sizeof(int64) * TextInput.length()));
    if (text == nullptr) return 0;
    for (size_t i = 0; i < TextInput.length(); i++)
        *(text + i) = Symbol[TextInput[i]];
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency() / 2) + 1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    Ort::Session sessionEncoder(env, (Path + L"_encoder.onnx").c_str(), session_options);
    Ort::Session sessionDecoderIter(env, (Path + L"_decoder_iter.onnx").c_str(), session_options);
    Ort::Session sessionPostNet(env, (Path + L"_postnet.onnx").c_str(), session_options);
    Ort::Session sessionGan(env, HifiganPath.c_str(), session_options);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    int64 textLength[] = { static_cast<int64>(TextInput.length()) };
    std::vector<MTensor> outputTensors;
    std::vector<MTensor> inputTensors;
    int64 inputShape1[2] = { 1,textLength[0] };
    int64 inputShape2[1] = { 1 };
    inputTensors.push_back(MTensor::CreateTensor<int64>(
        memory_info, text, textLength[0], inputShape1, 2));
    inputTensors.push_back(MTensor::CreateTensor<int64>(
        memory_info, textLength, 1, inputShape2, 1));
    outputTensors = sessionEncoder.Run(Ort::RunOptions{ nullptr },
        inputNodeNamesSessionEncoder.data(),
        inputTensors.data(),
        inputTensors.size(),
        outputNodeNamesSessionEncoder.data(),
        outputNodeNamesSessionEncoder.size());
    inputTensors = initDecoderInputs(outputTensors, memory_info);
    int32 notFinished = 1;
    int32 melLengths = 0;
    std::vector<MTensor> melGateAlig;
    melGateAlig.push_back(STensor::getZero<float>({ 1i64 }, memory_info));
    melGateAlig.push_back(STensor::getZero<float>({ 1i64 }, memory_info));
    melGateAlig.push_back(STensor::getZero<float>({ 1i64 }, memory_info));
    bool firstIter = true;
    while (true) {
        outputTensors = sessionDecoderIter.Run(Ort::RunOptions{ nullptr },
            inputNodeNamesSessionDecoderIter.data(),
            inputTensors.data(),
            inputTensors.size(),
            outputNodeNamesSessionDecoderIter.data(),
            outputNodeNamesSessionDecoderIter.size());
        if (firstIter) {
            melGateAlig[0] = STensor::unsqueezeCopy<float>(outputTensors[0], 2, 1, memory_info);
            melGateAlig[1] = STensor::unsqueezeCopy<float>(outputTensors[1], 2, 1, memory_info);
            melGateAlig[2] = STensor::unsqueezeCopy<float>(outputTensors[6], 2, 1, memory_info);
            firstIter = false;
        }
        else {
            melGateAlig[0] = STensor::cat1(melGateAlig[0], STensor::unsqueezeNoCopy<float>(outputTensors[0], 2, 1, memory_info), memory_info);
            melGateAlig[1] = STensor::cat1(melGateAlig[1], STensor::unsqueezeNoCopy<float>(outputTensors[1], 2, 1, memory_info), memory_info);
            melGateAlig[2] = STensor::cat1(melGateAlig[2], STensor::unsqueezeNoCopy<float>(outputTensors[6], 2, 1, memory_info), memory_info);
        }
        MTensor decTmp = STensor::unsqueezeNoCopy<float>(outputTensors[1], 2, 1, memory_info);
        MTensor dec = STensor::leCpp(STensor::sigmoid<float>(decTmp, memory_info), gateThreshold, memory_info);
        bool decInt = dec.GetTensorData<bool>()[0];
        notFinished = decInt * notFinished;
        melLengths += notFinished;
        if (!notFinished) {
            break;
        }
        else if (melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape()[2] == maxDecoderSteps) {
            FILE* decoderStepsOut = nullptr;
            decoderStepsOut = fopen("decoder", "w+");
            if (fclose(decoderStepsOut))
                return 0;
            return 1;
        }
        inputTensors[0] = std::move(outputTensors[0]);
        inputTensors[1] = std::move(outputTensors[2]);
        inputTensors[2] = std::move(outputTensors[3]);
        inputTensors[3] = std::move(outputTensors[4]);
        inputTensors[4] = std::move(outputTensors[5]);
        inputTensors[5] = std::move(outputTensors[6]);
        inputTensors[6] = std::move(outputTensors[7]);
        inputTensors[7] = std::move(outputTensors[8]);
    }
    std::vector<int64> melInputShape = melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<MTensor> melInput;
    melInput.push_back(MTensor::CreateTensor<float>(
        memory_info, melGateAlig[0].GetTensorMutableData<float>(), melInputShape[0] * melInputShape[1] * melInputShape[2], melInputShape.data(), melInputShape.size()));
    outputTensors = sessionPostNet.Run(Ort::RunOptions{ nullptr },
        inputNodeNamesSessionPostNet.data(),
        melInput.data(),
        melInput.size(),
        outputNodeNamesSessionPostNet.data(),
        outputNodeNamesSessionPostNet.size());
    std::vector<MTensor> wavOuts;
    wavOuts = sessionGan.Run(Ort::RunOptions{ nullptr },
        ganIn.data(),
        outputTensors.data(),
        outputTensors.size(),
        ganOut.data(),
        ganOut.size());
    std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
    auto TempVecWav = static_cast<int16_t*>(malloc(sizeof(int16_t) * wavOutsSharp[2]));
    if (TempVecWav == nullptr) return 0;
    for (int i = 0; i < wavOutsSharp[2]; i++) {
        *(TempVecWav + i) = static_cast<int16_t>(wavOuts[0].GetTensorData<float>()[i] * 32768.0f);
    }
    std::string fileNames = "tmpDir\\" + to_byte_string(OutDir) + ".wav";
    return conArr2Wav(wavOutsSharp[2], TempVecWav, fileNames.c_str());
}