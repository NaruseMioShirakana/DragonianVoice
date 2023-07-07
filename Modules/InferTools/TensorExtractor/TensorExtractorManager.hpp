#pragma once
#include <functional>
#include "MoeVoiceStudioTensorExtractor.hpp"


MoeVoiceStudioTensorExtractorHeader

class TensorExtractor
{
public:
	TensorExtractor() = default;
	TensorExtractor(MoeVoiceStudioTensorExtractor* _ext) : _f0_ext(_ext) {}
	TensorExtractor(const TensorExtractor&) = delete;
	TensorExtractor(TensorExtractor&& _ext) noexcept
	{
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
	}
	TensorExtractor& operator=(const TensorExtractor&) = delete;
	TensorExtractor& operator=(TensorExtractor&& _ext) noexcept
	{
		if (this == &_ext)
			return *this;
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
		return *this;
	}
	~TensorExtractor()
	{
		delete _f0_ext;
		_f0_ext = nullptr;
	}
	MoeVoiceStudioTensorExtractor* operator->() const { return _f0_ext; }

private:
	MoeVoiceStudioTensorExtractor* _f0_ext = nullptr;
};

using GetTensorExtractorFn = std::function<TensorExtractor(uint64_t, uint64_t, uint64_t, bool, bool, uint64_t, uint64_t, const MoeVoiceStudioTensorExtractor::Others&)>;

void RegisterTensorExtractor(const std::wstring& _name, const GetTensorExtractorFn& _constructor_fn);

TensorExtractor GetTensorExtractor(
	const std::wstring& _name,
	uint64_t _srcsr, uint64_t _sr, uint64_t _hop,
	bool _smix, bool _volume, uint64_t _hidden_size,
	uint64_t _nspeaker,
	const MoeVoiceStudioTensorExtractor::Others& _other
);

MoeVoiceStudioTensorExtractorEnd