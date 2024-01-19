#include <regex>
#include "Modules/StringPreprocess.hpp"
#ifndef MOEVSONNX
#include <deque>
#include <mutex>
#include <iostream>
#include <windows.h>
#include "LibDLVoiceCodec/value.h"
#pragma comment(lib, "winmm.lib") 

#ifdef _IOSTREAM_
std::ostream& operator<<(std::ostream& stream, const std::wstring& str)
{
	return stream << to_byte_string(str);
}
#include <vector>
template<typename T>
std::ostream& operator<<(std::ostream& stream, std::vector<T>& vec)
{
	stream << "[ ";
	for (size_t i = 0; i < vec.size(); ++i)
	{
		stream << vec[i];
		if (i != vec.size() - 1)
			stream << ", ";
	}
	stream << " ]";
	return stream;
}
#endif
#ifdef _VECTOR_
template <typename T>
std::vector<T>& operator-=(std::vector<T>& left, const std::vector<T>& right)
{
	for (size_t i = 0; i < left.size() && i < right.size(); ++i)
		left[i] -= right[i];
	return left;
}
#endif

int main()
{
	libdlvcodec::Tensor a({1,2,3});
}
#endif

/*MoeVSModuleManager::MoeVoiceStudioCoreInitSetup();

	DlCodecStft::Mel(2048, 512, 32000, 128)(InferTools::arange(0, 1, 0.00001));

	try
	{
		MoeVSModuleManager::LoadVitsSvcModel(
			MJson(to_byte_string(GetCurrentFolder() + L"/Models/crs.json").c_str()),
			[](size_t cur, size_t total)
			{
				std::cout << (double(cur) / double(total) * 100.) << "%\n";
			},
			0,
			8,
			0
			);
		MoeVSModuleManager::LoadDiffusionSvcModel(
			MJson(to_byte_string(GetCurrentFolder() + L"/Models/ShallowDiffusion.json").c_str()),
			[](size_t cur, size_t total)
			{
				std::cout << (double(cur) / double(total) * 100.) << "%\n";
			},
			0,
			8,
			0
		);
		MoeVSModuleManager::LoadVocoderModel(L"S:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Release\\hifigan\\nsf_hifigan.onnx");
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
		return 0;
	}

	MoeVSProjectSpace::MoeVSSvcParams Params;
	Params.Sampler = L"DDim";
	Params.Step = 100;
	Params.Pndm = 5;
	InferTools::SlicerSettings Settings;
	Params.F0Method = L"Dio";
	//Settings.SamplingRate = MoeVSModuleManager::GetCurSvcModel()->GetSamplingRate();
	Params.Keys = 0;
	std::wstring Paths;

	//MoeVSModuleManager::GetCurSvcModel()->Inference(Paths, Params, Settings);

	return 0;

	const MJson Config(to_byte_string(GetCurrentFolder() + L"/Models/HimenoSena.json").c_str());  //改为模型配置路径（相对exe）
	const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback ProCallback = [](size_t cur, size_t total)
	{
		std::cout << (double(cur) / double(total) * 100.) << "%\n";
	};
	const MoeVoiceStudioCore::TextToSpeech::DurationCallback DurCallback = [](std::vector<float>&)
	{
		return;
	};
	try
	{
		const MoeVoiceStudioCore::TextToSpeech* VitsTest = dynamic_cast<MoeVoiceStudioCore::TextToSpeech*>(new MoeVoiceStudioCore::Vits(Config, ProCallback, DurCallback, MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders::CPU, 8, 0));
		//这里改为Json的字符串或者Json文件
		const auto Voice = VitsTest->Inference(MJson("S:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\test.json"));
		//输出
		InferTools::Wav::WritePCMData(VitsTest->GetSamplingRate(), 1, Voice[0], L"Test1.wav");
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}

	return 0;*/