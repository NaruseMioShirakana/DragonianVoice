#include <iostream>
#include "header/ModelBase.hpp"
#include "header/VitsSvc.hpp"
#include <chrono>

rapidjson::Document parseJSONFile(const std::string& filename) {
    rapidjson::Document document;

    // 打开JSON文件
    std::ifstream file(filename);

    if (file.is_open()) {
        // 将文件内容读入字符串
        std::string jsonContent((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        // 解析JSON字符串
        document.Parse(jsonContent.c_str());

        // 检查解析是否成功
        if (document.HasParseError()) {
            std::cout << "Failed to parse JSON." << std::endl;
        }

        // 关闭文件
        file.close();
    }
    else {
        //std::cout << "Failed to open JSON file." << std::endl;
        throw std::exception("Failed to open JSON file.\n");
    }

    return document;
}

InferClass::OnnxModule* CreateModel(const InferClass::OnnxModule::callback_params& ParamsCallback) {
    std::string modelPath;
    std::cout << "input onnx model" << std::endl;
    std::cin >> modelPath;

    std::string configPath;
    std::cout << "input config model" << std::endl;
    std::cin >> configPath;

    rapidjson::Document Config = parseJSONFile(configPath);
    // 添加字符串字段到Document对象
    rapidjson::Document::AllocatorType& allocator = Config.GetAllocator();
    rapidjson::Value key("ModelPath", allocator);
    rapidjson::Value value(modelPath.c_str(), allocator);
    Config.AddMember(key, value, allocator);

    //Progress bar
    const InferClass::OnnxModule::callback ProgressCallback = [](size_t a, size_t b) {std::cout << std::to_string((float)a * 100.f / (float)b) << "%\n"; };

    const auto model = dynamic_cast<InferClass::OnnxModule*>(
        new InferClass::VitsSvc(Config, ProgressCallback, ParamsCallback)
        );
    return model;
}

InferClass::OnnxModule* TryCreateModel(const InferClass::OnnxModule::callback_params& ParamsCallback) {

    try
    {
        return CreateModel(ParamsCallback);
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
        return TryCreateModel(ParamsCallback);
    }

}

std::string extractContent(const std::string& str, std::string& remainingStr) {
    std::size_t first = str.find('{');
    std::size_t last = str.rfind('}');

    if (first != std::string::npos && last != std::string::npos) {
        remainingStr = str.substr(0, first) + str.substr(last + 1);
        return str.substr(first, last - first + 1);
    }
    else {
        remainingStr = str;
        return "";
    }
}

void SetInterConfigFromJson(InferClass::InferConfigs& config, const std::string& json) {

    rapidjson::Document interConfigParam;
    //TODO
    // 解析JSON字符串
    try {
        interConfigParam.Parse(json.c_str());

        // 检查解析是否成功
        if (!interConfigParam.HasParseError()) {
            if (interConfigParam.HasMember("keys")) {
                config.keys = interConfigParam["keys"].GetInt64();
            }
            if (interConfigParam.HasMember("kmeans_rate")) {
                config.kmeans_rate = interConfigParam["kmeans_rate"].GetInt64();
            }
        }
    }
    catch (std::exception& e){
        std::cout << e.what();
    }
}

int main() {
    //rapidjson::Document Config;
 //   Config.Parse(R"({
    //"Folder" : "march7",
    //"Name" : "march7",
    //"Type" : "RVC",
    //"Rate" : 48000,
    //"Hop" : 320,
    //"Cleaner" : "",
    //"Hubert": "hubert4.0",
    //"Diffusion": false,
    //"CharaMix": false,
    //"Volume": false,
    //"HiddenSize": 256,
    //"Characters" : ["march7"]
	//})");

    auto configInfer = InferClass::InferConfigs();
    configInfer.kmeans_rate = 0.5f;
    configInfer.keys = 0;


    //return params for inference
    const InferClass::OnnxModule::callback_params ParamsCallback = [&configInfer]()  // 注意这里的改动
    {
        auto params = InferClass::InferConfigs();
        params.kmeans_rate = configInfer.kmeans_rate;
        params.keys = configInfer.keys;
        return params;
    };


    const auto model = TryCreateModel(ParamsCallback);


    //modify duration per phoneme
    InferClass::TTS::DurationCallback DurationCallback = [](std::vector<float>&) {};

    std::wstring exit = L"exit";
    std::vector<int16_t> output;
    try
    {
        //std::wstring inp;
        do
        {
            std::wstring input;
            std::wstring outpath;

            std::wcout << "input source audio path " << std::endl;
            std::wcin >> input;

            if (input.empty() || input.size() == 0) {
                continue;
            }

            if (input.compare(exit) == 0 ) {
                break;
            }

            std::string extractedJsonContent, inputWavPath;
            extractedJsonContent = extractContent(to_byte_string(input), inputWavPath);
            if (!extractedJsonContent.empty() && extractedJsonContent.size() > 0) {
                SetInterConfigFromJson(configInfer, extractedJsonContent);
            }

            std::wcout << "out audio path " << std::endl;
            std::wcin >> outpath;

            if (outpath.empty() || outpath.size() == 0) {
                continue;
            }
            // 在这里可以根据输入指令进行相应的处理
            //std::wcout << "输入指令为：" << input << std::endl;


            // 记录开始时间
            auto start = std::chrono::high_resolution_clock::now();
            output = model->Inference(to_wide_string(inputWavPath));


            const Wav outWav(model->GetSamplingRate(), output.size() * 2, output.data());
            outWav.Writef(outpath);
            std::wcout << "saveed audio path " << outpath << std::endl;

            // 记录结束时间
            auto end = std::chrono::high_resolution_clock::now();

            // 计算执行时间
            std::chrono::duration<double, std::milli> elapsed = end - start;

            // 输出执行时间
            std::cout << "rvc cost time: " << elapsed.count() << " ms" << std::endl;

        } while (true);

    }
    catch (std::exception& e)
    {
        std::cout << e.what();
    }

    delete model;

    std::cout << "MoeVioce Done " << std::endl;
}