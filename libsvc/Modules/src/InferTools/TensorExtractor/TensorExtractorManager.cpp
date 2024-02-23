#include "../../../header/InferTools/TensorExtractor/TensorExtractorManager.hpp"
#include <map>
#include "../../../header/Logger/MoeSSLogger.hpp"

MoeVoiceStudioTensorExtractorHeader
	inline std::map<std::wstring, GetTensorExtractorFn> RegisteredTensorExtractors;

void RegisterTensorExtractor(const std::wstring& _name, const GetTensorExtractorFn& _constructor_fn)
{
	if (RegisteredTensorExtractors.find(_name) != RegisteredTensorExtractors.end())
	{
		logger.log(L"[Warn] TensorExtractorNameConflict");
		return;
	}
	RegisteredTensorExtractors[_name] = _constructor_fn;
}

TensorExtractor GetTensorExtractor(const std::wstring& _name, uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const MoeVoiceStudioTensorExtractor::Others& _other)
{
	const auto f_TensorExtractor = RegisteredTensorExtractors.find(_name);
	if (f_TensorExtractor != RegisteredTensorExtractors.end())
		return f_TensorExtractor->second(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other);
	throw std::runtime_error("Unable To Find An Available TensorExtractor");
}

MoeVoiceStudioTensorExtractorEnd