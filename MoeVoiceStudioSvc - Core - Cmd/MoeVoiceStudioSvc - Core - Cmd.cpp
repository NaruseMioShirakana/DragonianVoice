
#include <iostream>
#include "Modules/Modules.hpp"
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

	MoeVSModuleManager::LoadSvcModel(
		MJson(to_byte_string(GetCurrentFolder() + L"/Models/ShirohaRVC.json").c_str()),
		[](size_t cur, size_t total) {std::cout << (double(cur) / double(total) * 100.) << "%\n"; },
		0,
		std::thread::hardware_concurrency(),
		0
	);

	MoeVSProjectSpace::MoeVSSvcParams Params;
	InferTools::SlicerSettings Settings;
	Params.F0Method = L"Harvest";
	Settings.Threshold = 30.;
	try
	{
		std::wstring Paths;
		auto tmp = MoeVSModuleManager::GetCurSvcModel()->Inference(Paths, Params, Settings);
		std::cout << tmp;
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}