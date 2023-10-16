#include "MoeVoiceStudioTensorExtractor.hpp"
#include "../inferTools.hpp"
MoeVoiceStudioTensorExtractorHeader

MoeVoiceStudioTensorExtractor::MoeVoiceStudioTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other)
{
	_SrcSamplingRate = _srcsr;
	_SamplingRate = _sr;
	_HopSize = _hop;
	_SpeakerMix = _smix;
	_Volume = _volume;
	_HiddenSize = _hidden_size;
	_NSpeaker = _nspeaker;
	f0_bin = _other.f0_bin;
	f0_max = _other.f0_max;
	f0_min = _other.f0_min;
	f0_mel_min = 1127.f * log(1.f + f0_min / 700.f);
	f0_mel_max = 1127.f * log(1.f + f0_max / 700.f);
	Memory = _other.Memory;
}

MoeVoiceStudioTensorExtractor::Inputs MoeVoiceStudioTensorExtractor::Extract(
	const std::vector<float>& HiddenUnit,
	const std::vector<float>& F0,
	const std::vector<float>& Volume,
	const std::vector<std::vector<float>>& SpkMap,
	Params params
)
{
	throw std::exception("NotImplementedError");
}

std::vector<float> MoeVoiceStudioTensorExtractor::GetCurrectSpkMixData(const std::vector<std::vector<float>>& _input, size_t dst_len, int64_t curspk) const
{
	std::vector<float> mixData;
	mixData.reserve(_NSpeaker * dst_len);
	if(_input.empty())
	{
		std::vector<float> LenData(_NSpeaker, 0.0);
		LenData[curspk] = 1.0;
		for (size_t i = 0; i < dst_len; ++i)
			mixData.insert(mixData.end(), LenData.begin(), LenData.end());
	}
	else
	{
		std::vector<std::vector<float>> _spkMap;
		for (size_t i = 0; i < _input.size() && i < _NSpeaker; ++i)
			_spkMap.emplace_back(InferTools::InterpFunc(_input[i], long(_input[i].size()), long(dst_len)));
		LinearCombination(_spkMap, curspk);
		const auto curnspk = _input.size();
		if (curnspk < _NSpeaker)
		{
			std::vector<float> LenData(_NSpeaker - curnspk, 0.0);
			for (size_t i = 0; i < dst_len; ++i)
			{
				for (size_t j = 0; j < curnspk; ++j)
					mixData.emplace_back(_spkMap[j][i]);
				mixData.insert(mixData.end(), LenData.begin(), LenData.end());
			}
		}
		else
			for (size_t i = 0; i < dst_len; ++i)
				for (size_t j = 0; j < _NSpeaker; ++j)
					mixData.emplace_back(_spkMap[j][i]);
	}
	return mixData;
}

std::vector<float> MoeVoiceStudioTensorExtractor::GetSpkMixData(const std::vector<std::vector<float>>& _input, size_t dst_len, size_t spk_count)
{
	std::vector<float> mixData;
	mixData.reserve(spk_count * dst_len);
	if (_input.empty())
	{
		std::vector<float> LenData(spk_count, 0.0);
		LenData[0] = 1.0;
		for (size_t i = 0; i < dst_len; ++i)
			mixData.insert(mixData.end(), LenData.begin(), LenData.end());
	}
	else
	{
		std::vector<std::vector<float>> _spkMap;
		for (size_t i = 0; i < _input.size() && i < spk_count; ++i)
			_spkMap.emplace_back(InferTools::InterpFunc(_input[i], long(_input[i].size()), long(dst_len)));
		LinearCombination(_spkMap, 0);
		const auto curnspk = _input.size();
		if (curnspk < spk_count)
		{
			std::vector<float> LenData(spk_count - curnspk, 0.0);
			for (size_t i = 0; i < dst_len; ++i)
			{
				for (size_t j = 0; j < curnspk; ++j)
					mixData.emplace_back(_spkMap[j][i]);
				mixData.insert(mixData.end(), LenData.begin(), LenData.end());
			}
		}
		else
			for (size_t i = 0; i < dst_len; ++i)
				for (size_t j = 0; j < spk_count; ++j)
					mixData.emplace_back(_spkMap[j][i]);
	}
	return mixData;
}

std::vector<int64_t> MoeVoiceStudioTensorExtractor::GetNSFF0(const std::vector<float>& F0) const
{
	const auto f0Len = F0.size();
	std::vector<int64_t> NSFF0(f0Len);
	for (size_t i = 0; i < f0Len; ++i)
	{
		float f0_mel = 1127.f * log(1.f + F0[i] / 700.f);
		if (f0_mel > 0.f)
			f0_mel = (f0_mel - f0_mel_min) * (float(f0_bin) - 2.f) / (f0_mel_max - f0_mel_min) + 1.f;
		if (f0_mel < 1.f)
			f0_mel = 1.f;
		if (f0_mel > float(f0_bin) - 1.f)
			f0_mel = float(f0_bin) - 1.f;
		NSFF0[i] = (int64_t)round(f0_mel);
	}
	return NSFF0;
}

std::vector<float> MoeVoiceStudioTensorExtractor::GetInterpedF0(const std::vector<float>& F0)
{
	const auto specLen = F0.size();
	std::vector<float> Of0(specLen, 0.0);

	float last_value = 0.0;
	for (size_t i = 0; i < specLen; ++i)
	{
		if (F0[i] <= 0.f)
		{
			size_t j = i + 1;
			for (; j < specLen; ++j)
			{
				if (F0[j] > 0.f)
					break;
			}
			if (j < specLen - 1)
			{
				if (last_value > 0.f)
				{
					const auto step = (F0[j] - F0[i - 1]) / float(j - i);
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[i - 1] + step * float(k - i + 1));
				}
				else
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[j]);
				i = j;
			}
			else
			{
				for (size_t k = i; k < specLen; ++k)
					Of0[k] = float(last_value);
				i = specLen;
			}
		}
		else
		{
			if (i == 0)
			{
				Of0[i] = float(F0[i]);
				continue;
			}
			Of0[i] = float(F0[i - 1]);
			last_value = F0[i];
		}
	}
	return Of0;
}

std::vector<float> MoeVoiceStudioTensorExtractor::GetUV(const std::vector<float>& F0)
{
	const auto specLen = F0.size();
	std::vector<float> ruv(specLen, 1.0);
	for (size_t i = 0; i < specLen; ++i)
	{
		if (F0[i] < 0.001f)
			ruv[i] = 0.f;
	}
	return ruv;
}

std::vector<int64_t> MoeVoiceStudioTensorExtractor::GetAligments(size_t specLen, size_t hubertLen)
{
	std::vector<int64_t> mel2ph(specLen + 1, 0);

	size_t startFrame = 0;
	const double ph_durs = static_cast<double>(specLen) / static_cast<double>(hubertLen);
	for (size_t iph = 0; iph < hubertLen; ++iph)
	{
		const auto endFrame = static_cast<size_t>(round(static_cast<double>(iph) * ph_durs + ph_durs));
		for (auto j = startFrame; j < endFrame + 1; ++j)
			mel2ph[j] = static_cast<long long>(iph) + 1;
		startFrame = endFrame + 1;
	}
	return mel2ph;
}

std::vector<float> MoeVoiceStudioTensorExtractor::GetInterpedF0log(const std::vector<float>& rF0, bool enable_log)
{
	const auto specLen = rF0.size();
	std::vector<float> F0(specLen);
	std::vector<float> Of0(specLen, 0.0);
	for (size_t i = 0; i < specLen; ++i)
	{
		if (enable_log)
			F0[i] = log2(rF0[i]);
		else
			F0[i] = rF0[i];
		if (isnan(F0[i]) || isinf(F0[i]))
			F0[i] = 0.f;
	}

	float last_value = 0.0;
	for (size_t i = 0; i < specLen; ++i)
	{
		if (F0[i] <= 0.f)
		{
			size_t j = i + 1;
			for (; j < specLen; ++j)
			{
				if (F0[j] > 0.f)
					break;
			}
			if (j < specLen - 1)
			{
				if (last_value > 0.f)
				{
					const auto step = (F0[j] - F0[i - 1]) / float(j - i);
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[i - 1] + step * float(k - i + 1));
				}
				else
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[j]);
				i = j;
			}
			else
			{
				for (size_t k = i; k < specLen; ++k)
					Of0[k] = float(last_value);
				i = specLen;
			}
		}
		else
		{
			Of0[i] = float(F0[i - 1]);
			last_value = F0[i];
		}
	}
	return Of0;
}

MoeVoiceStudioTensorExtractorEnd