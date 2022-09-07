#pragma warning(disable : 4996)
#include <iostream>
#include <D:\VisualStudioProj\Tacotron inside\MioShirakanaTensor.hpp>
#include <locale>
#include <codecvt>
#include <io.h>
#include <map>
#include <direct.h>
#include <fstream>
#include <thread>
using std::cout;
using std::endl;
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
    head.size0 = size * 2 + 36;
    head.size2 = size * 2;
    std::ofstream ocout;
    char* outputData = (char*)input;
    ocout.open(filename, std::ios::out | std::ios::binary);
    ocout.write((char*)&head, 44);
    ocout.write(outputData, (int32_t)(size * 2));
    ocout.close();
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

std::vector<MTensor> init_decoder_inputs(std::vector<MTensor>& inputTens, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> tmparrshira0 = inputTens[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> tmparrshira1 = inputTens[1].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> attention_rnn_dim{ tmparrshira0[0],1024 };
    std::vector<int64> decoder_rnn_dim{ tmparrshira0[0],1024 };
    std::vector<int64> encoder_embedding_dim{ tmparrshira0[0],512 };
    std::vector<int64> n_mel_channels{ tmparrshira0[0],80 };
    std::vector<int64> seqLen{ tmparrshira0[0],tmparrshira0[1] };
    std::vector<MTensor> outTensorVec;
    bool* tempBoolean = (bool*)malloc(sizeof(bool) * seqLen[0] * seqLen[1]);
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
        outTensorVec.push_back(STensor::fromArray(inputTens[0].GetTensorMutableData<float>(), tmparrshira0, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(inputTens[1].GetTensorMutableData<float>(), tmparrshira1, memoryInfo));
        outTensorVec.push_back(STensor::fromArray(tempBoolean, seqLen, memoryInfo));
    }
    catch (Ort::Exception e) {
        cout << e.what();
    }
    return outTensorVec;
}

int main(int argc, char* argv[]) {

    if (argc != 5) {
        return 0;
    }
    wchar_t* buffer;
    if ((buffer = _wgetcwd(NULL, 0)) == NULL) {
        perror("无法获取路径");
        return 0;
    }
    /*
    std::wstring PathTmp = to_wide_string(argv[1]);
    std::wstring Path = bufferStr + L"\\" + PathTmp;
    std::wstring TextInput = to_wide_string(argv[2]);
    std::wstring OutDir = to_wide_string(argv[3]);
    std::wstring HifiganPath = bufferStr + L"\\hifigan.onnx";
    std::wstring SymbolStr = to_wide_string(argv[4]);
    */
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
    int64* text = (int64*)malloc(sizeof(int64) * TextInput.length());
    if (text == NULL) return 0;
    for (size_t i = 0; i < TextInput.length(); i++) {
        *(text + i) = Symbol[TextInput[i]];
    }
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
    Ort::SessionOptions session_options;
    OrtCUDAProviderOptions cuda_options;
    cuda_options.device_id = 0;
    cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
    cuda_options.gpu_mem_limit = static_cast<int>(SIZE_MAX * 1024 * 1024);
    cuda_options.arena_extend_strategy = 1;
    cuda_options.do_copy_in_default_stream = 1;
    cuda_options.has_user_compute_stream = 1;
    cuda_options.default_memory_arena_cfg = nullptr;
    session_options.AppendExecutionProvider_CUDA(cuda_options);
    session_options.SetIntraOpNumThreads((int)(std::thread::hardware_concurrency() / 2) + 1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    Ort::Session session(env, (Path + L"_encoder.onnx").c_str(), session_options);
    Ort::Session session_decoder_iter(env, (Path + L"_decoder_iter.onnx").c_str(), session_options);
    Ort::Session session_postnet(env, (Path + L"_postnet.onnx").c_str(), session_options);
    Ort::Session session_gan(env, HifiganPath.c_str(), session_options);
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    int64 textLength[] = { TextInput.length() };
    std::vector<const char*> input_node_names = { "sequences","sequence_lengths" };
    std::vector<const char*> output_node_names = { "memory","processed_memory","lens" };
    std::vector<const char*> input_node_names_session_decoder_iter = { "decoder_input","attention_hidden","attention_cell","decoder_hidden","decoder_cell","attention_weights","attention_weights_cum","attention_context","memory","processed_memory","mask" };
    std::vector<const char*> output_node_names_session_decoder_iter = { "decoder_output","gate_prediction","out_attention_hidden","out_attention_cell","out_decoder_hidden","out_decoder_cell","out_attention_weights","out_attention_weights_cum","out_attention_context" };
    std::vector<const char*> input_node_names_session_postnet = { "mel_outputs" };
    std::vector<const char*> output_node_names_session_postnet = { "mel_outputs_postnet" };
    std::vector<MTensor> output_tensors;
    std::vector<MTensor> output_tensors_session_decoder_iter;
    std::vector<MTensor> output_tensors_session_postnet;
    std::vector<MTensor> input_tensors;
    std::array<int64_t, 2> input_shape_1{ 1,textLength[0] };
    std::array<int64_t, 1> input_shape_2{ 1 };
    try {
        input_tensors.push_back(MTensor::CreateTensor<int64>(
            memory_info, text, textLength[0], input_shape_1.data(), input_shape_1.size()));
        input_tensors.push_back(MTensor::CreateTensor<int64>(
            memory_info, textLength, 1, input_shape_2.data(), input_shape_2.size()));
    }
    catch (Ort::Exception e) {
        return 0;
    }
    output_tensors = session.Run(Ort::RunOptions{ nullptr },
        input_node_names.data(),
        input_tensors.data(),
        input_tensors.size(),
        output_node_names.data(),
        output_node_names.size());
    input_tensors = init_decoder_inputs(output_tensors, memory_info);
    int32_t not_finished = 1;
    int32_t mel_lengths = 0;
    std::vector<int64_t> input_shape_MGA{ 1 };
    std::vector<MTensor> melGateAlig;
    melGateAlig.push_back(STensor::getZero<float>(input_shape_MGA, memory_info));
    melGateAlig.push_back(STensor::getZero<float>(input_shape_MGA, memory_info));
    melGateAlig.push_back(STensor::getZero<float>(input_shape_MGA, memory_info));
    float gate_threshold = 0.666;
    int64 max_decoder_steps = 5000;
    bool firstIter = true;
    while (true) {
        output_tensors_session_decoder_iter = session_decoder_iter.Run(Ort::RunOptions{ nullptr },
            input_node_names_session_decoder_iter.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_node_names_session_decoder_iter.data(),
            output_node_names_session_decoder_iter.size());
        if (firstIter) {
            melGateAlig[0] = STensor::unsqueezeCopy<float>(output_tensors_session_decoder_iter[0], 2, 1, memory_info);
            melGateAlig[1] = STensor::unsqueezeCopy<float>(output_tensors_session_decoder_iter[1], 2, 1, memory_info);
            melGateAlig[2] = STensor::unsqueezeCopy<float>(output_tensors_session_decoder_iter[6], 2, 1, memory_info);
            firstIter = false;
        }
        else {
            melGateAlig[0] = STensor::cat1(melGateAlig[0], STensor::unsqueezeNoCopy<float>(output_tensors_session_decoder_iter[0], 2, 1, memory_info), memory_info);
            melGateAlig[1] = STensor::cat1(melGateAlig[1], STensor::unsqueezeNoCopy<float>(output_tensors_session_decoder_iter[1], 2, 1, memory_info), memory_info);
            melGateAlig[2] = STensor::cat1(melGateAlig[2], STensor::unsqueezeNoCopy<float>(output_tensors_session_decoder_iter[6], 2, 1, memory_info), memory_info);
        }
        MTensor decTmp = STensor::unsqueezeNoCopy<float>(output_tensors_session_decoder_iter[1], 2, 1, memory_info);
        MTensor dec = STensor::leCpp(STensor::sigmoid<float>(decTmp, memory_info), gate_threshold, memory_info);
        bool dec_int = dec.GetTensorData<bool>()[0];
        not_finished = dec_int * not_finished;
        mel_lengths += not_finished;
        if (not_finished == 0) {
            break;
        }
        else if (melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape()[2] == max_decoder_steps) {
            FILE* decoderStepsOut = nullptr;
            decoderStepsOut = fopen("decoder", "w+");
            fclose(decoderStepsOut);
            return 0;
        }
        input_tensors[0] = std::move(output_tensors_session_decoder_iter[0]);
        input_tensors[1] = std::move(output_tensors_session_decoder_iter[2]);
        input_tensors[2] = std::move(output_tensors_session_decoder_iter[3]);
        input_tensors[3] = std::move(output_tensors_session_decoder_iter[4]);
        input_tensors[4] = std::move(output_tensors_session_decoder_iter[5]);
        input_tensors[5] = std::move(output_tensors_session_decoder_iter[6]);
        input_tensors[6] = std::move(output_tensors_session_decoder_iter[7]);
        input_tensors[7] = std::move(output_tensors_session_decoder_iter[8]);
    }
    std::vector<int64> melInputShape = melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<MTensor> melInput;
    melInput.push_back(MTensor::CreateTensor<float>(
        memory_info, melGateAlig[0].GetTensorMutableData<float>(), melInputShape[0] * melInputShape[1] * melInputShape[2], melInputShape.data(), melInputShape.size()));
    output_tensors_session_postnet = session_postnet.Run(Ort::RunOptions{ nullptr },
        input_node_names_session_postnet.data(),
        melInput.data(),
        melInput.size(),
        output_node_names_session_postnet.data(),
        output_node_names_session_postnet.size());
    std::vector<const char*> gan_in = { "x" };
    std::vector<const char*> gan_out = { "audio" };
    std::vector<MTensor> wavOuts;
    wavOuts = session_gan.Run(Ort::RunOptions{ nullptr },
        gan_in.data(),
        output_tensors_session_postnet.data(),
        output_tensors_session_postnet.size(),
        gan_out.data(),
        gan_out.size());
    std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
    std::array<int64, 1> SharpIN{ wavOutsSharp[2] };
    int16_t* TempVecWav = (int16_t*)malloc(sizeof(int16_t) * wavOutsSharp[2]);
    if (TempVecWav == NULL) return 0;
    for (int i = 0; i < wavOutsSharp[2]; i++) {
        *(TempVecWav + i) = (int16_t)(wavOuts[0].GetTensorData<float>()[i] * 32768.0);
    }
    std::string filenames = "tmpDir\\" + to_byte_string(OutDir) + ".wav";
    return conArr2Wav(wavOutsSharp[2], TempVecWav, filenames.c_str());
}