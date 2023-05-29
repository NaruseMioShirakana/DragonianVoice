#include <iostream>

#include "header/VitsSvc.hpp"

int main() {
    rapidjson::Document Config;
    Config.Parse(R"({
        "Folder" : "ShirohaRVC",
        "Name" : "Shiroha-RVC",
        "Type" : "RVC",
        "Symbol" : "",
        "Cleaner" : "",
        "Rate" : 40000,
        "Hop" : 320,
        "Hifigan" : "",
        "Hubert" : "hubert4.0",
        "Characters" : ["Shiroha"]
	})");

    //Progress bar
    const InferClass::BaseModelType::callback ProgressCallback = [](size_t a, size_t b) {std::cout << std::to_string((float)a * 100.f / (float)b) << "%\n"; };

    //return params for inference
    const InferClass::BaseModelType::callback_params ParamsCallback = []()
    {
        auto params = InferClass::InferConfigs();
        params.kmeans_rate = 0.5;
        params.keys = 0;
        return params;
    };

    //modify duration per phoneme
    InferClass::TTS::DurationCallback DurationCallback = [](std::vector<float>&) {};

    std::vector<int16_t> output;
    try
    {
        std::wstring inp;
        const auto model = dynamic_cast<InferClass::BaseModelType*>(
            new InferClass::VitsSvc(Config, ProgressCallback, ParamsCallback)
            );

        output = model->Inference(inp);


        std::wstring outpath = GetCurrentFolder() + L"\\test.wav";
        const Wav outWav(model->GetSamplingRate(), output.size() * 2, output.data());
        outWav.Writef(outpath);

        delete model;
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
    }
}