#pragma once
#include "MoeVSBaseSampler.hpp"

MoeVoiceStudioSamplerHeader

class MoeVSSampler
{
public:
	MoeVSSampler() = delete;
	MoeVSSampler(MoeVSBaseSampler* _ext) : _f0_ext(_ext) {}
	MoeVSSampler(const MoeVSSampler&) = delete;
	MoeVSSampler(MoeVSSampler&& _ext) noexcept
	{
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
	}
	MoeVSSampler& operator=(const MoeVSSampler&) = delete;
	MoeVSSampler& operator=(MoeVSSampler&& _ext) noexcept
	{
		if (this == &_ext)
			return *this;
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
		return *this;
	}
	~MoeVSSampler()
	{
		delete _f0_ext;
		_f0_ext = nullptr;
	}
	MoeVSBaseSampler* operator->() const { return _f0_ext; }

private:
	MoeVSBaseSampler* _f0_ext = nullptr;
};

using GetMoeVSSamplerFn = std::function<MoeVSSampler(Ort::Session*, Ort::Session*, Ort::Session*, int64_t, const MoeVSBaseSampler::ProgressCallback&, Ort::MemoryInfo*)>;

void RegisterMoeVSSampler(const std::wstring& _name, const GetMoeVSSamplerFn& _constructor_fn);

MoeVSSampler GetMoeVSSampler(const std::wstring& _name,
	Ort::Session* alpha,
	Ort::Session* dfn,
	Ort::Session* pred,
	int64_t Mel_Bins,
	const MoeVSBaseSampler::ProgressCallback& _ProgressCallback,
	Ort::MemoryInfo* memory);

std::vector<std::wstring> GetMoeVSSamplerList();

MoeVoiceStudioSamplerEnd