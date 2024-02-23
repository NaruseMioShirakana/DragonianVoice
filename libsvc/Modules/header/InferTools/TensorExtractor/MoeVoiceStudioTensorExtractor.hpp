/**
 * FileName: MoeVoiceStudioTensorExtractor.hpp
 * Note: MoeVoiceStudioCore 张量预处理基类
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2022-10-17 Create
*/

#pragma once
#include <string>
#include <vector>
#include "onnxruntime_cxx_api.h"
#define MoeVoiceStudioTensorExtractorHeader namespace MoeVSTensorPreprocess{
#define MoeVoiceStudioTensorExtractorEnd }
#include "../../framework.h"

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

	/**
	 * \brief 构造张量预处理器
	 * \param _srcsr 原始采样率
	 * \param _sr 目标采样率
	 * \param _hop HopSize
	 * \param _smix 是否启用角色混合
	 * \param _volume 是否启用音量emb
	 * \param _hidden_size hubert的维数
	 * \param _nspeaker 角色数
	 * \param _other 其他参数，其中的memoryInfo必须为你当前模型的memoryInfo
	 */
	MoeVoiceStudioTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other);

	virtual ~MoeVoiceStudioTensorExtractor() = default;

	/**
	 * \brief 预处理张量
	 * \param HiddenUnit HiddenUnit
	 * \param F0 F0
	 * \param Volume 音量
	 * \param SpkMap 角色混合数据
	 * \param params 参数
	 * \return 完成预处理的张量（请将张量接管的所有Vector的数据都存储到Tensors Data中，因为ORT创建的张量要求调用方管理内存，如果不存储到这个位置会导致数据提前析构
	 */
	virtual Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	);

	void SetSrcSamplingRates(uint64_t sr) { _SrcSamplingRate = sr; }

	//获取换算为0-255的f0
	[[nodiscard]] std::vector<int64_t> GetNSFF0(const std::vector<float>&) const;

	//将F0中0值单独插值
	static std::vector<float> GetInterpedF0(const std::vector<float>&);

	//
	static std::vector<float> InterpUVF0(const std::vector<float>&);

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
	LibSvcApi [[nodiscard]] static std::vector<float> GetInterpedF0log(const std::vector<float>&, bool);

	//获取正确的角色混合数据
	[[nodiscard]] std::vector<float> GetCurrectSpkMixData(const std::vector<std::vector<float>>& _input, size_t dst_len, int64_t curspk) const;

	//获取正确的角色混合数据
	LibSvcApi [[nodiscard]] static std::vector<float> GetSpkMixData(const std::vector<std::vector<float>>& _input, size_t dst_len, size_t spk_count);
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