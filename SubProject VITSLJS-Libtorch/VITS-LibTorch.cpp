
#include <iostream>
#include <torch/torch.h>
#include <torch/script.h>
#include <string>
#include <vector>
#include <cmath>
#include <locale>
#include <codecvt>
#include <io.h>
#include <direct.h>
#include <fstream>
#include <thread>
#include <map>
#include <D:\VisualStudioProj\ShirakanaClass\[Twilight-Dream]StringTypeConversion.hpp>
typedef int64_t int64;

namespace Shirakana {

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
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(input);
    }

    inline std::string to_byte_string(const std::wstring& input)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(input);
    }
}

int main(int argc,char* argv[])
{
    if (argc != 6 && argc != 7) {
        return 0;
    }
    wchar_t* buffer;
    if ((buffer = _wgetcwd(NULL, 0)) == NULL) {
        perror("无法获取路径");
        return 0;
    }
    std::wstring bufferStr = buffer;
    std::wstring PathTmp = string2wstring(argv[1]);
    std::wstring Path = bufferStr + L"\\" + PathTmp;
    std::wstring TextInput = string2wstring(argv[2]);
    std::wstring OutDir = string2wstring(argv[3]);
    std::wstring SymbolStr = string2wstring(argv[4]);
    std::string Mode = argv[5];
    std::map<wchar_t, int64> Symbol;
    for (size_t i = 0; i < SymbolStr.length(); i++) {
        Symbol.insert(std::pair<wchar_t, int64>(SymbolStr[i], (int64)(i)));
    }
    std::vector<int64> text;
    for (size_t i = 0; i < TextInput.length(); i++) {
        text.push_back(0);
        text.push_back(Symbol[TextInput[i]]);
    }
    text.push_back(0);
    auto InputTensor = torch::from_blob(text.data(), { 1, (long long)text.size() }, torch::kInt64);
    std::array<int64, 1> TextLength{ text.size() };
    auto InputTensor_length = torch::from_blob(TextLength.data(), { 1 }, torch::kInt64);
    torch::jit::script::Module VITSMODULE;
    std::vector<torch::IValue> inputs;
    inputs.push_back(InputTensor);
    inputs.push_back(InputTensor_length);
    if (Mode == "LJS") {
        try {
            VITSMODULE = torch::jit::load(Shirakana::to_byte_string(PathTmp) + "_LJS.pt");
        }
        catch (c10::Error e) {
            std::cout << e.what();
            return 0;
        }
    }
    else if(Mode == "VCTK") {
        if (argc != 7) {
            return 0;
        }
        try {
            std::array<int64, 1> speakerIndex{ (int64)atoi(argv[6]) };
            VITSMODULE = torch::jit::load(Shirakana::to_byte_string(PathTmp) + "_VCTK.pt");
            inputs.push_back(torch::from_blob(speakerIndex.data(), { 1 }, torch::kLong));
        }
        catch (c10::Error e) {
            std::cout << e.what();
            return 0;
        }
    }
    else {
        std::cout << "模式错误";
    }
    try {
        auto output = VITSMODULE.forward(inputs).toTuple()->elements()[0].toTensor().multiply(32276.0F);
        auto outputSize = output.sizes().at(2);
        auto floatOutput = output.data<float>();
        int16_t* outputTmp = (int16_t*)malloc(sizeof(float) * outputSize);
        if (outputTmp == NULL) {
            throw std::exception("内存不足");
        }
        for (int i = 0; i < outputSize; i++) {
            *(outputTmp + i) = (int16_t) * (floatOutput + i);
        }
        std::string filenames = "tmpDir\\" + Shirakana::to_byte_string(OutDir) + ".wav";
        return Shirakana::conArr2Wav(outputSize, outputTmp, filenames.c_str());
    }
    catch (c10::Error e) {
        std::cout << e.what();
        return 0;
    }
}
