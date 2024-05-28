#include "F0ExtractorManager.hpp"
#include <map>
#include <stdexcept>
#include "../../Logger/MoeSSLogger.hpp"

MoeVoiceStudioF0ExtractorHeader
std::map<std::wstring, GetF0ExtractorFn> RegisteredF0Extractors;

F0Extractor GetF0Extractor(const std::wstring& _name,
	const uint32_t fs,
	const uint32_t hop,
	const uint32_t f0_bin,
	const double f0_max,
	const double f0_min)
{
	const auto f_F0Extractor = RegisteredF0Extractors.find(_name);
	if (f_F0Extractor != RegisteredF0Extractors.end())
		return f_F0Extractor->second(fs, hop, f0_bin, f0_max, f0_min);
	throw std::runtime_error("Unable To Find An Available F0Extractor");
}

void RegisterF0Extractor(const std::wstring& _name, const GetF0ExtractorFn& _constructor_fn)
{
	if (RegisteredF0Extractors.find(_name) != RegisteredF0Extractors.end())
	{
		logger.log(L"[Warn] F0ExtractorNameConflict");
		return;
	}
	RegisteredF0Extractors[_name] = _constructor_fn;
}

std::vector<std::wstring> GetF0ExtractorList()
{
	std::vector<std::wstring> F0ExtractorsVec;
	F0ExtractorsVec.reserve(RegisteredF0Extractors.size());
	for (const auto& i : RegisteredF0Extractors)
		F0ExtractorsVec.emplace_back(i.first);
	return F0ExtractorsVec;
}

MoeVoiceStudioF0ExtractorEnd