#include "../../../header/InferTools/Sampler/MoeVSSamplerManager.hpp"
#include <map>
#include "../../../header/Logger/MoeSSLogger.hpp"
MoeVoiceStudioSamplerHeader
std::map<std::wstring, GetMoeVSSamplerFn> RegisteredMoeVSSamplers;
std::map<std::wstring, GetMoeVSReflowSamplerFn> RegisteredMoeVSReflowSamplers;

MoeVSSampler GetMoeVSSampler(const std::wstring& _name,
	Ort::Session* alpha,
	Ort::Session* dfn,
	Ort::Session* pred,
	int64_t Mel_Bins,
	const MoeVSBaseSampler::ProgressCallback& _ProgressCallback,
	Ort::MemoryInfo* memory)
{
	const auto f_Sampler = RegisteredMoeVSSamplers.find(_name);
	if (f_Sampler != RegisteredMoeVSSamplers.end())
		return f_Sampler->second(alpha, dfn, pred, Mel_Bins, _ProgressCallback, memory);
	throw std::runtime_error("Unable To Find An Available Sampler");
}

void RegisterMoeVSSampler(const std::wstring& _name, const GetMoeVSSamplerFn& _constructor_fn)
{
	if (RegisteredMoeVSSamplers.find(_name) != RegisteredMoeVSSamplers.end())
	{
		logger.log(L"[Warn] SamplerNameConflict");
		return;
	}
	RegisteredMoeVSSamplers[_name] = _constructor_fn;
}

std::vector<std::wstring> GetMoeVSSamplerList()
{
	std::vector<std::wstring> SamplersVec;
	SamplersVec.reserve(RegisteredMoeVSSamplers.size());
	for (const auto& i : RegisteredMoeVSSamplers)
		SamplersVec.emplace_back(i.first);
	return SamplersVec;
}

MoeVSReflowSampler GetMoeVSReflowSampler(
	const std::wstring& _name, 
	Ort::Session* velocity, 
	int64_t Mel_Bins, 
	const MoeVSBaseSampler::ProgressCallback& _ProgressCallback, 
	Ort::MemoryInfo* memory
)
{
	const auto f_Sampler = RegisteredMoeVSReflowSamplers.find(_name);
	if (f_Sampler != RegisteredMoeVSReflowSamplers.end())
		return f_Sampler->second(velocity, Mel_Bins, _ProgressCallback, memory);
	throw std::runtime_error("Unable To Find An Available Sampler");
}

void RegisterMoeVSReflowSampler(const std::wstring& _name, const GetMoeVSReflowSamplerFn& _constructor_fn)
{
	if (RegisteredMoeVSReflowSamplers.find(_name) != RegisteredMoeVSReflowSamplers.end())
	{
		logger.log(L"[Warn] SamplerNameConflict");
		return;
	}
	RegisteredMoeVSReflowSamplers[_name] = _constructor_fn;
}

std::vector<std::wstring> GetMoeVSReflowSamplerList()
{
	std::vector<std::wstring> SamplersVec;
	SamplersVec.reserve(RegisteredMoeVSReflowSamplers.size());
	for (const auto& i : RegisteredMoeVSReflowSamplers)
		SamplersVec.emplace_back(i.first);
	return SamplersVec;
}

MoeVoiceStudioSamplerEnd