#pragma warning(disable : 4996)
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <cmath>
#include <locale>
#include <codecvt>
#include <io.h>
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

int conArr2Wav(int64 size,int16_t* input,const char* filename) {
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
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(input);
}
// convert wstring to string 
inline std::string to_byte_string(const std::wstring& input)
{
    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(input);
}

float* getZero(int64 m) {
    float* zeroMartix = (float*)malloc(sizeof(float) * m);
    if (zeroMartix == NULL) {
        throw std::exception("Null");
        return nullptr;
    }
    for (int i = 0; i < m; i++) {
        *(zeroMartix + i) = (float)0;
    }
    return zeroMartix;
}

std::vector<Ort::Value> squeeze(std::vector<Ort::Value> inputTenA) {
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    std::array<int64_t, 1> shape{ inputTenA[0].GetTensorTypeAndShapeInfo().GetShape()[2] };
    int32_t* TenOut = (int32_t*)malloc(sizeof(int32_t) * shape[0]);
    if (TenOut == NULL) {
        throw std::exception("Null");
    }
    for (int i = 0; i < shape[0]; i++) {
        *(TenOut + i) = inputTenA[0].GetTensorMutableData<int32_t>()[i];
    }
    std::vector<Ort::Value> OutPutTens;
    OutPutTens.push_back(Ort::Value::CreateTensor<int32_t>(
        memory_info, TenOut, shape[0], shape.data(), shape.size()));
    return OutPutTens;
}

std::vector<Ort::Value> leCpp(float* inputTenA, float inputB , std::vector<int64_t> Ten) {
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    int32_t* TenOut = (int32_t*)malloc(sizeof(int32_t) * Ten[0] * Ten[1] * Ten[2]);
    if (TenOut == NULL) {
        throw std::exception("Null");
    }
    for (int i = 0; i < Ten[0] * Ten[1] * Ten[2]; i++) {
        if (inputTenA[i] <= inputB) *(TenOut + i) = 1;
        else *(TenOut + i) = 0;
    }
    std::vector<Ort::Value> OutPutTens;
    OutPutTens.push_back(Ort::Value::CreateTensor<int32_t>(
        memory_info, TenOut, Ten[0] * Ten[1] * Ten[2], Ten.data(), Ten.size()));
    return OutPutTens;
}

std::vector<Ort::Value> sigmoid(float* TenIN, std::vector<int64_t> Ten) {
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    float* TenOut = (float*)malloc(sizeof(float) * Ten[0] * Ten[1] * Ten[2]);
    if (TenOut == NULL) {
        throw std::exception("Null");
    }
    for (int i = 0; i < Ten[0] * Ten[1] * Ten[2]; i++) {
        *(TenOut + i) = 1.0F/(1.0F + exp(0.0F - TenIN[i]));
    }
    std::vector<Ort::Value> OutPutTens;
    OutPutTens.push_back(Ort::Value::CreateTensor<float>(
        memory_info, TenOut, Ten[0] * Ten[1] * Ten[2], Ten.data(), Ten.size()));
    return OutPutTens;
}

std::vector<Ort::Value> catTens32(std::vector<Ort::Value>& inputTensA, std::vector<Ort::Value>& inputTensB) {
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    std::vector<int64_t> Ten0A = inputTensA[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64_t> Ten1A = inputTensA[1].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64_t> Ten2A = inputTensA[2].GetTensorTypeAndShapeInfo().GetShape();
    Ten0A[2]++;
    Ten1A[2]++;
    Ten2A[2]++;
    float* Ten0Out = (float*)malloc(sizeof(float) * Ten0A[0] * Ten0A[1] * Ten0A[2]);
    float* Ten1Out = (float*)malloc(sizeof(float) * Ten1A[0] * Ten1A[1] * Ten1A[2]);
    float* Ten2Out = (float*)malloc(sizeof(float) * Ten2A[0] * Ten2A[1] * Ten2A[2]);
    for (int i = 0, k = 0; i < 80; i++) {
        for (int j = 0; j < Ten0A[2] - 1; j++) {
            *(Ten0Out + k) = inputTensA[0].GetTensorData<float>()[(i * (Ten0A[2] - 1)) + j];
            k++;
        }
        *(Ten0Out + k) = inputTensB[0].GetTensorData<float>()[i];
        k++;
    }
    for (int i = 0; i < Ten1A[2] - 1; i++) {
        *(Ten1Out + i) = inputTensA[1].GetTensorData<float>()[i];
    }
    *(Ten1Out + Ten1A[2]-1) = inputTensB[1].GetTensorData<float>()[0];
    for (int i = 0, k = 0; i < Ten2A[1]; i++) {
        for (int j = 0; j < Ten2A[2] - 1; j++) {
            *(Ten2Out + k) = inputTensA[2].GetTensorData<float>()[(i * (Ten2A[2]-1)) + j];
            k++;
        }
        *(Ten2Out + k) = inputTensB[2].GetTensorData<float>()[i];
        k++;
    }
    std::vector<Ort::Value> outTens;
    try {
        outTens.push_back(Ort::Value::CreateTensor<float>(
            memory_info, Ten0Out, Ten0A[0] * Ten0A[1] * Ten0A[2], Ten0A.data(), Ten0A.size()));
        outTens.push_back(Ort::Value::CreateTensor<float>(
            memory_info, Ten1Out, Ten1A[0] * Ten1A[1] * Ten1A[2], Ten1A.data(), Ten1A.size()));
        outTens.push_back(Ort::Value::CreateTensor<float>(
            memory_info, Ten2Out, Ten2A[0] * Ten2A[1] * Ten2A[2], Ten2A.data(), Ten2A.size()));
    }
    catch (Ort::Exception e) {
        cout << e.what();
        return outTens;
    }
    return outTens;
}

std::vector<Ort::Value> unsqueeze(std::vector<Ort::Value>& inputTens) {
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    std::vector<int64_t> Ten0 = inputTens[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64_t> Ten1 = inputTens[1].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64_t> Ten6 = inputTens[6].GetTensorTypeAndShapeInfo().GetShape();
    Ten0.push_back(1);
    Ten1.push_back(1);
    Ten6.push_back(1);
    std::vector<Ort::Value> outTens;
    float* Ten0Vce = (float*)malloc(sizeof(float) * Ten0[0] * Ten0[1] * Ten0[2]);
    float* Ten1Vce = (float*)malloc(sizeof(float) * Ten1[0] * Ten1[1] * Ten1[2]);
    float* Ten6Vce = (float*)malloc(sizeof(float) * Ten6[0] * Ten6[1] * Ten6[2]);
    if (Ten0Vce == NULL || Ten1Vce == NULL || Ten6Vce == NULL) {
        throw std::exception("Null");
        return outTens;
    }
    for (int i = 0; i < Ten0[0] * Ten0[1] * Ten0[2]; i++)
        *(Ten0Vce + i) = inputTens[0].GetTensorData<float>()[i];
    for (int i = 0; i < Ten1[0] * Ten1[1] * Ten1[2]; i++)
        *(Ten1Vce + i) = inputTens[1].GetTensorData<float>()[i];
    for (int i = 0; i < Ten6[0] * Ten6[1] * Ten6[2]; i++)
        *(Ten6Vce + i) = inputTens[6].GetTensorData<float>()[i];
    try {
        outTens.push_back(Ort::Value::CreateTensor<float>(
            memory_info, Ten0Vce, Ten0[0] * Ten0[1] * Ten0[2], Ten0.data(), Ten0.size()));
        outTens.push_back(Ort::Value::CreateTensor<float>(
            memory_info, Ten1Vce, Ten1[0] * Ten1[1] * Ten1[2], Ten1.data(), Ten1.size()));
        outTens.push_back(Ort::Value::CreateTensor<float>(
            memory_info, Ten6Vce, Ten6[0] * Ten6[1] * Ten6[2], Ten6.data(), Ten6.size()));
    }
    catch (Ort::Exception e) {
        cout << e.what(); 
        return outTens;
    }
    return outTens;
}

std::vector<Ort::Value> init_decoder_inputs(std::vector<Ort::Value>& inputTens) {
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    std::vector<int64> tmparrshira0 = inputTens[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> tmparrshira1 = inputTens[1].GetTensorTypeAndShapeInfo().GetShape();
    std::array<int64_t, 2> attention_rnn_dim{ tmparrshira0[0],1024 };
    std::array<int64_t, 2> decoder_rnn_dim{ tmparrshira0[0],1024 };
    std::array<int64_t, 2> encoder_embedding_dim{ tmparrshira0[0],512 };
    std::array<int64_t, 2> n_mel_channels{ tmparrshira0[0],80 };
    std::array<int64_t, 2> seqLen{ tmparrshira0[0],tmparrshira0[1] };
    std::vector<Ort::Value> outTensorVec;
    bool* tempBoolean = (bool*)malloc(sizeof(bool) * seqLen[0] * seqLen[1]);
    if (tempBoolean == NULL) {
        throw std::exception("Null");
        return outTensorVec;
    }
    for (int i = 0; i < seqLen[0] * seqLen[1]; i++) {
        *(tempBoolean + i) = false;
    }
    try {
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(n_mel_channels[0] * n_mel_channels[1]), n_mel_channels[0] * n_mel_channels[1], n_mel_channels.data(), n_mel_channels.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(attention_rnn_dim[0] * attention_rnn_dim[1]), attention_rnn_dim[0] * attention_rnn_dim[1], attention_rnn_dim.data(), attention_rnn_dim.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(attention_rnn_dim[0] * attention_rnn_dim[1]), attention_rnn_dim[0] * attention_rnn_dim[1], attention_rnn_dim.data(), attention_rnn_dim.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(decoder_rnn_dim[0] * decoder_rnn_dim[1]), decoder_rnn_dim[0] * decoder_rnn_dim[1], decoder_rnn_dim.data(), decoder_rnn_dim.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(decoder_rnn_dim[0] * decoder_rnn_dim[1]), decoder_rnn_dim[0] * decoder_rnn_dim[1], decoder_rnn_dim.data(), decoder_rnn_dim.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(seqLen[0] * seqLen[1]), seqLen[0] * seqLen[1], seqLen.data(), seqLen.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(seqLen[0] * seqLen[1]), seqLen[0] * seqLen[1], seqLen.data(), seqLen.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, getZero(encoder_embedding_dim[0] * encoder_embedding_dim[1]), encoder_embedding_dim[0] * encoder_embedding_dim[1], encoder_embedding_dim.data(), encoder_embedding_dim.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, inputTens[0].GetTensorMutableData<float>(), tmparrshira0[0]* tmparrshira0[1]* tmparrshira0[2], tmparrshira0.data(), tmparrshira0.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<float>(
            memory_info, inputTens[1].GetTensorMutableData<float>(), tmparrshira0[0] * tmparrshira0[1] * tmparrshira0[2], tmparrshira1.data(), tmparrshira1.size()));
        outTensorVec.push_back(Ort::Value::CreateTensor<bool>(
            memory_info, tempBoolean, seqLen[0] * seqLen[1], seqLen.data(), seqLen.size()));
    }
    catch (Ort::Exception e) {
        cout << "错误：" <<e.what();
    }
    return outTensorVec;
}

void printModelInfo(Ort::Session& session, Ort::AllocatorWithDefaultOptions& allocator)
{
    //输出模型输入节点的数量
    size_t num_input_nodes = session.GetInputCount();
    size_t num_output_nodes = session.GetOutputCount();
    cout << "Number of input node is:" << num_input_nodes << endl;
    cout << "Number of output node is:" << num_output_nodes << endl;

    //获取输入输出维度
    for (auto i = 0; i < num_input_nodes; i++)
    {
        std::vector<int64_t> input_dims = session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        cout << endl << "input " << i << " dim is: ";
        for (auto j = 0; j < input_dims.size(); j++)
            cout << input_dims[j] << " ";
    }
    for (auto i = 0; i < num_output_nodes; i++)
    {
        std::vector<int64_t> output_dims = session.GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        cout << endl << "output " << i << " dim is: ";
        for (auto j = 0; j < output_dims.size(); j++)
            cout << output_dims[j] << " ";
    }
    //输入输出的节点名
    cout << endl;//换行输出
    for (auto i = 0; i < num_input_nodes; i++)
        cout << "The input op-name " << i << " is:" << session.GetInputName(i, allocator) << endl;
    for (auto i = 0; i < num_output_nodes; i++)
        cout << "The output op-name " << i << " is:" << session.GetOutputName(i, allocator) << endl;

    //input_dims_2[0] = input_dims_1[0] = output_dims[0] = 1;//batch size = 1
}




int main(int argc, char* argv[])
{
    
    /*

    


    std::string OutDir = "0";
    std::wstring HifiganPath = to_wide_string("D:\\VisualStudioProj\\Tacotron inside\\x64\\Release\\hifigan.onnx");
    std::wstring Path = L"D:\\VisualStudioProj\\Tacotron inside\\x64\\Release\\Mods\\Shiroha\\Shiroha";
    std::string TextInput = "watashihanaruseshirohawatashihanaruseshirohawatashihanaruseshirohawatashihanaruseshiroha.";
    
    */



    if (argc != 4) {
        return 0;
    }
    char* buffer;
    //也可以将buffer作为输出参数
    if ((buffer = getcwd(NULL, 0)) == NULL) {
        perror("无法获取路径");
        return 0;
    }
    std::string bufferStr = buffer;
    std::string PathTmp = argv[1];
    std::wstring Path = to_wide_string(bufferStr + "\\" + PathTmp);
    std::string TextInput = argv[2];
    std::string OutDir = argv[3];
    std::wstring HifiganPath = to_wide_string(bufferStr + "\\hifigan.onnx");




    //std::wcout << Path;
    int64* text = (int64*)malloc(sizeof(int64) * TextInput.length());
    if (text == NULL) {
        return 0;
    }
    for (size_t i = 0; i < TextInput.length(); i++) {
        if (TextInput[i] <= 'Z' && TextInput[i] >= 'A') TextInput[i] += 32;
        if (TextInput[i] == ' ') {
            text[i] = (int64)11;
        }
        else if (TextInput[i] == '.') {
            text[i] = (int64)7;
        }
        else {
            text[i] = (int64)((int64)TextInput[i] - (int64)'a') + (int64)38;
        }
    }


	Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
	Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads((int)(std::thread::hardware_concurrency() / 2) + 1);
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

	//***************************************************************************************************

	Ort::Session session(env, (Path + L"_encoder.onnx").c_str(), session_options);
    Ort::Session session_decoder_iter(env, (Path + L"_decoder_iter.onnx").c_str(), session_options);
    Ort::Session session_postnet(env, (Path + L"_postnet.onnx").c_str(), session_options);
    Ort::Session session_gan(env, HifiganPath.c_str(), session_options);

    //***************************************************************************************************

	Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    int64 textLength[] = { TextInput.length() };
    std::vector<const char*> input_node_names = { "sequences","sequence_lengths" };
    std::vector<const char*> output_node_names = { "memory","processed_memory","lens"};
    std::vector<const char*> input_node_names_session_decoder_iter = { "decoder_input","attention_hidden","attention_cell","decoder_hidden","decoder_cell","attention_weights","attention_weights_cum","attention_context","memory","processed_memory","mask" };
    std::vector<const char*> output_node_names_session_decoder_iter = { "decoder_output","gate_prediction","out_attention_hidden","out_attention_cell","out_decoder_hidden","out_decoder_cell","out_attention_weights","out_attention_weights_cum","out_attention_context" };
    std::vector<const char*> input_node_names_session_postnet = { "mel_outputs" };
    std::vector<const char*> output_node_names_session_postnet = { "mel_outputs_postnet" };
    std::vector<Ort::Value> output_tensors;
    std::vector<Ort::Value> output_tensors_session_decoder_iter;
    std::vector<Ort::Value> output_tensors_session_postnet;
    std::vector<Ort::Value> input_tensors;
    std::array<int64_t, 2> input_shape_1{ 1,textLength[0] };
    std::array<int64_t, 1> input_shape_2{ 1 };
    try {
        input_tensors.push_back(Ort::Value::CreateTensor<int64>(
            memory_info, text, textLength[0], input_shape_1.data(), input_shape_1.size()));
        input_tensors.push_back(Ort::Value::CreateTensor<int64>(
            memory_info, textLength, 1, input_shape_2.data(), input_shape_2.size()));
    }
    catch (Ort::Exception e) {
        cout << e.what();
        return 0;
    }

    //***************************************************************************************************

    output_tensors = session.Run(Ort::RunOptions{ nullptr },
        input_node_names.data(),
        input_tensors.data(),
        input_tensors.size(),
        output_node_names.data(),
        output_node_names.size());
    input_tensors = init_decoder_inputs(output_tensors);

    //***************************************************************************************************
    int32_t not_finished = 1;
    int32_t mel_lengths = 1;
    std::array<int64_t, 1> input_shape_MGA{ 1 };
    std::vector<Ort::Value> melGateAlig;
    std::vector<Ort::Value> melGateAligTmp;
    melGateAlig.push_back(Ort::Value::CreateTensor<float>(
        memory_info, getZero(1), 1 , input_shape_MGA.data(), input_shape_MGA.size()));
    melGateAlig.push_back(Ort::Value::CreateTensor<float>(
        memory_info, getZero(1), 1 , input_shape_MGA.data(), input_shape_MGA.size()));
    melGateAlig.push_back(Ort::Value::CreateTensor<float>(
        memory_info, getZero(1), 1 , input_shape_MGA.data(), input_shape_MGA.size()));
    float gate_threshold = 0.6;
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
            melGateAligTmp = unsqueeze(output_tensors_session_decoder_iter);
            melGateAlig = unsqueeze(output_tensors_session_decoder_iter);
            firstIter = false;
        }
        else {
            melGateAligTmp = unsqueeze(output_tensors_session_decoder_iter);
            melGateAlig = catTens32(melGateAlig, melGateAligTmp);
        }
        std::vector<Ort::Value> decTmp = sigmoid(melGateAligTmp[1].GetTensorMutableData<float>(), melGateAligTmp[1].GetTensorTypeAndShapeInfo().GetShape());
        std::vector<Ort::Value> dec = leCpp(decTmp[0].GetTensorMutableData<float>(), gate_threshold , decTmp[0].GetTensorTypeAndShapeInfo().GetShape());
        int32_t dec_int = dec[0].GetTensorData<int32_t>()[0];
        not_finished = dec_int * not_finished;
        mel_lengths += not_finished;
        if (not_finished == 0) {
            break;
        }else if(melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape()[2] == max_decoder_steps){
            cout << "Warning! Reached max decoder steps";
            FILE* decoderStepsOut = nullptr;
            decoderStepsOut = fopen("decoder", "w+");
            fclose(decoderStepsOut);
            break;
        }
        try {
            std::vector<int64_t> TempSizeInfo = output_tensors_session_decoder_iter[0].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[0] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[0].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[2].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[1] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[2].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[3].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[2] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[3].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[4].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[3] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[4].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[5].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[4] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[5].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[6].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[5] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[6].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[7].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[6] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[7].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
            
            TempSizeInfo = output_tensors_session_decoder_iter[8].GetTensorTypeAndShapeInfo().GetShape();
            input_tensors[7] = Ort::Value::CreateTensor<float>(
                memory_info, output_tensors_session_decoder_iter[8].GetTensorMutableData<float>(), TempSizeInfo[0] * TempSizeInfo[1], TempSizeInfo.data(), TempSizeInfo.size());
        }
        catch (Ort::Exception e) {
            cout << e.what();
        }
    }
    std::vector<int64> melInputShape = melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape();
    std::vector<Ort::Value> melInput;
    melInput.push_back(Ort::Value::CreateTensor<float>(
        memory_info, melGateAlig[0].GetTensorMutableData<float>(), melInputShape[0]* melInputShape[1]* melInputShape[2], melInputShape.data(), melInputShape.size()));
    output_tensors_session_postnet = session_postnet.Run(Ort::RunOptions{ nullptr },
        input_node_names_session_postnet.data(),
        melInput.data(),
        melInput.size(),
        output_node_names_session_postnet.data(),
        output_node_names_session_postnet.size());
    std::vector<const char*> gan_in = { "x" };
    std::vector<const char*> gan_out = { "audio" };
    std::vector<Ort::Value> wavOuts;
    wavOuts = session_gan.Run(Ort::RunOptions{ nullptr },
        gan_in.data(),
        output_tensors_session_postnet.data(),
        output_tensors_session_postnet.size(),
        gan_out.data(),
        gan_out.size());
    std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
    std::array<int64, 1> SharpIN{ wavOutsSharp[2] };
    int16_t* TempVecWav = (int16_t*)malloc(sizeof(int16_t)* wavOutsSharp[2]);
    for (int i = 0; i < wavOutsSharp[2]; i++) {
        *(TempVecWav+i) = (int16_t)(wavOuts[0].GetTensorData<float>()[i] * 32768.0);
    }
    cout << "c";
    std::string filenames = "tmpDir\\" + OutDir + ".wav";
    return conArr2Wav(wavOutsSharp[2], TempVecWav, filenames.c_str());
}
