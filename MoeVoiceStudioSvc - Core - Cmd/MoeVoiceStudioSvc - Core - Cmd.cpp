
#include <iostream>
#include "Modules/Modules.hpp"
#include "Modules/AvCodec/AvCodeResample.h"
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

int main()
{
	MoeVSModuleManager::MoeVoiceStudioCoreInitSetup();

	try
	{
		MoeVSModuleManager::LoadSvcModel(
			MJson(to_byte_string(GetCurrentFolder() + L"/Models/ShirohaRVC.json").c_str()),
			[](size_t cur, size_t total)
			{
				//std::cout << (double(cur) / double(total) * 100.) << "%\n";
			},
			1,
			8,
			0
			);
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
	Params.F0Method = L"RMVPE";
	Settings.SamplingRate = 40000;
#ifdef DEBUGUSETRYCATCH
	try
	{
#endif
	std::wstring Paths;
	auto TPCMData = AudioPreprocess().codec(LR"(S:\VSGIT\MoeSS - Release\Testdata\123.wav)", Settings.SamplingRate);
	//MoeVSModuleManager::GetCurSvcModel()->InferPCMData(TPCMData, Settings.SamplingRate, Params);
	std::vector<int16_t> PCMData = { TPCMData.begin(),TPCMData.begin() + Settings.SamplingRate };
	std::vector<int16_t> _data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	PCMData = { TPCMData.begin() + Settings.SamplingRate * 1,TPCMData.begin() + Settings.SamplingRate * 2 };
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	PCMData = { TPCMData.begin() + Settings.SamplingRate * 2,TPCMData.begin() + Settings.SamplingRate * 3 };
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	PCMData = { TPCMData.begin() + Settings.SamplingRate * 3,TPCMData.begin() + Settings.SamplingRate * 4 };
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	PCMData = { TPCMData.begin() + Settings.SamplingRate * 4,TPCMData.begin() + Settings.SamplingRate * 5 };
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	PCMData = { TPCMData.begin() + Settings.SamplingRate * 5,TPCMData.begin() + Settings.SamplingRate * 6 };
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	PCMData = { TPCMData.begin() + Settings.SamplingRate * 6,TPCMData.begin() + Settings.SamplingRate * 7 };
	auto now = clock();
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	auto inferTime = double(clock() - now) / 1000.;
	std::cout << "Infer Use Time : " << inferTime << "sec.\n";
	PCMData = TPCMData;
	now = clock();
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	inferTime = double(clock() - now) / 1000.;
	std::cout << "Infer Use Time : " << inferTime << "sec.\n";
	now = clock();
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	inferTime = double(clock() - now) / 1000.;
	std::cout << "Infer Use Time : " << inferTime << "sec.\n";
	now = clock();
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	inferTime = double(clock() - now) / 1000.;
	std::cout << "Infer Use Time : " << inferTime << "sec.\n";
	now = clock();
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	inferTime = double(clock() - now) / 1000.;
	std::cout << "Infer Use Time : " << inferTime << "sec.\n";
	now = clock();
	_data = MoeVSModuleManager::GetCurSvcModel()->InferPCMData(PCMData, Settings.SamplingRate, Params);
	inferTime = double(clock() - now) / 1000.;
	std::cout << "Infer Use Time : " << inferTime << "sec.\n";
#ifdef DEBUGUSETRYCATCH
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	catch (Ort::Exception& e)
	{
		std::cout << e.what() << std::endl;
	}
#endif
	system("pause");
}