#pragma once
#include "BaseF0Extractor/BaseF0Extractor.hpp"
#include <functional>

MOEVSFOEXTRACTORHEADER

class F0Extractor
{
public:
	F0Extractor() = delete;
	F0Extractor(BaseF0Extractor* _ext) : _f0_ext(_ext) {}
	F0Extractor(const F0Extractor&) = delete;
	F0Extractor(F0Extractor&& _ext) noexcept
	{
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
	}
	F0Extractor& operator=(const F0Extractor&) = delete;
	F0Extractor& operator=(F0Extractor&& _ext) noexcept
	{
		if (this == &_ext)
			return *this;
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
		return *this;
	}
	~F0Extractor()
	{
		delete _f0_ext;
		_f0_ext = nullptr;
	}
	BaseF0Extractor* operator->() const { return _f0_ext; }

private:
	BaseF0Extractor* _f0_ext = nullptr;
};

using GetF0ExtractorFn = std::function<F0Extractor(uint32_t, uint32_t, uint32_t, double, double)>;

void RegisterF0Extractor(const std::wstring& _name, const GetF0ExtractorFn& _constructor_fn);

F0Extractor GetF0Extractor(const std::wstring& _name,
	uint32_t fs = 48000,
	uint32_t hop = 512,
	uint32_t f0_bin = 256,
	double f0_max = 1100.0,
	double f0_min = 50.0);

std::vector<std::wstring> GetF0ExtractorList();

MOEVSFOEXTRACTOREND