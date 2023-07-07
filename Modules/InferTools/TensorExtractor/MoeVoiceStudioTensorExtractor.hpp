#pragma once
#include <string>
#include <vector>
#include "onnxruntime_cxx_api.h"
#define MoeVoiceStudioTensorExtractorHeader namespace MoeVSTensorPreprocess{
#define MoeVoiceStudioTensorExtractorEnd }


MoeVoiceStudioTensorExtractorHeader

class MoeVoiceStudioTensorExtractor
{
public:
	struct Tensors
	{
		std::vector<float> HiddenUnit;
		std::vector<float> F0;
		std::vector<float> Volume;
		std::vector<float> SpkMap;
		std::vector<float> DDSPNoise;
		std::vector<float> Noise;
		std::vector<int64_t> Alignment;
		std::vector<float> UnVoice;
		std::vector<int64_t> NSFF0;
		int64_t Length[1] = { 0 };
		int64_t Speaker[1] = { 0 };

		std::vector<int64_t> HiddenUnitShape;
		std::vector<int64_t> FrameShape;
		std::vector<int64_t> SpkShape;
		std::vector<int64_t> DDSPNoiseShape;
		std::vector<int64_t> NoiseShape;
		int64_t OneShape[1] = { 1 };
	};

	struct InferParams
	{
		float NoiseScale = 0.3f;
		float DDSPNoiseScale = 1.0f;
		int Seed = 520468;
		size_t AudioSize = 0;
		int64_t Chara = 0;
		float upKeys = 0.f;
		void* Other = nullptr;
	};

	struct Others
	{
		int f0_bin = 256;
		float f0_max = 1100.0;
		float f0_min = 50.0;
		OrtMemoryInfo* Memory = nullptr;
		void* Other = nullptr;
	};

	using Params = const InferParams&;

	struct Inputs
	{
		Tensors Data;
		std::vector<Ort::Value> Tensor;
		const char* const* InputNames = nullptr;
		const char* const* OutputNames = nullptr;
		size_t InputCount = 1;
		size_t OutputCount = 1;
	};

	MoeVoiceStudioTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other);

	virtual ~MoeVoiceStudioTensorExtractor() = default;

	virtual Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	);

	//获取换算为0-255的f0
	[[nodiscard]] std::vector<int64_t> GetNSFF0(const std::vector<float>&) const;

	//将F0中0值单独插值
	static std::vector<float> GetInterpedF0(const std::vector<float>&);

	//获取UnVoiceMask
	static std::vector<float> GetUV(const std::vector<float>&);

	//获取对齐矩阵
	static std::vector<int64_t> GetAligments(size_t, size_t);

	//线性组合
	template <typename T>
	static void LinearCombination(std::vector<std::vector<T>>& _data, size_t default_id, T Value = T(1.0))
	{
		if (_data.empty())
			return;
		if (default_id > _data.size())
			default_id = 0;

		for (size_t i = 0; i < _data[0].size(); ++i)
		{
			T Sum = T(0.0);
			for (size_t j = 0; j < _data.size(); ++j)
				Sum += _data[j][i];
			if (Sum < T(0.0001))
			{
				for (size_t j = 0; j < _data.size(); ++j)
					_data[j][i] = T(0);
				_data[default_id][i] = T(1);
				continue;
			}
			Sum *= T(Value);
			for (size_t j = 0; j < _data.size(); ++j)
				_data[j][i] /= Sum;
		}
	}

	//将F0中0值单独插值（可设置是否取log）
	[[nodiscard]] static std::vector<float> GetInterpedF0log(const std::vector<float>&, bool);

	[[nodiscard]] std::vector<float> GetCurrectSpkMixData(const std::vector<std::vector<float>>& _input, size_t dst_len, int64_t curspk) const;
protected:
	uint64_t _NSpeaker = 1;
	uint64_t _SrcSamplingRate = 32000;
	uint64_t _SamplingRate = 32000;
	uint64_t _HopSize = 512;
	bool _SpeakerMix = false;
	bool _Volume = false;
	uint64_t _HiddenSize = 256;
	int f0_bin = 256;
	float f0_max = 1100.0;
	float f0_min = 50.0;
	float f0_mel_min = 1127.f * log(1.f + f0_min / 700.f);
	float f0_mel_max = 1127.f * log(1.f + f0_max / 700.f);
	OrtMemoryInfo* Memory = nullptr;
private:
	std::wstring __NAME__CLASS__ = L"MoeVoiceStudioTensorExtractor";
};

MoeVoiceStudioTensorExtractorEnd