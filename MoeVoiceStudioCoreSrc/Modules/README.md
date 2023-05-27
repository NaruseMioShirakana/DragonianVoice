# Example
```c++
#include "Modules/Models/header/Vits.hpp"

int main(){
  rapidjson::Document Config;
  Config.Parse("Your Config");
  
  //Progress bar
  InferClass::BaseModelType::callback a_callback = [](size_t a, size_t b) {std::cout << std::to_string((float)a * 100.f / (float)b) << "%\n"; };
  
  //return params for inference
  InferClass::BaseModelType::callback_params b_callback = []()  
	{
		auto cbaaa = InferClass::InferConfigs();
		cbaaa.kmeans_rate = 0.5;
		cbaaa.keys = 0;
		return cbaaa;
	};
  
  //modify duration per phoneme
  InferClass::TTS::DurationCallback c_callback = [](std::vector<float>&) {};
  
  std::vector<int16_t> output;
  try
  {
  	std::wstring inp;
  	auto model = dynamic_cast<InferClass::BaseModelType*>(new InferClass::VitsSvc(modConfigJson, a_callback, b_callback));
    
  	output = model->Inference(inp);
    
  	Wav outWav(model->GetSamplingRate(), output.size() * 2, output.data());
  	outWav.Writef(L"test.wav");
    
  	delete model;
  }
  catch(std::exception& e)
  {
  	std::cout << e.what();
  }
}

```
