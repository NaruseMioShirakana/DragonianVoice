/**
 * FileName: SVC.hpp
 * Note: MoeVoiceStudioCore OnnxSvc 模型基类
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
#include "ModelBase.hpp"
#include "../Logger/MoeSSLogger.hpp"
#include "../InferTools/inferTools.hpp"
#include "../InferTools/TensorExtractor/TensorExtractorManager.hpp"
#include "../InferTools/Cluster/MoeVSClusterManager.hpp"

MoeVoiceStudioCoreHeader

/*
class OnnxModule
{
public:
	enum class Device
	{
		CPU = 0,
		CUDA = 1,
#ifdef MOEVSDMLPROVIDER
		DML = 2
#endif
	};
	using callback = std::function<void(size_t, size_t)>;
	using int64 = int64_t;
	using MTensor = Ort::Value;

	OnnxModule();
	virtual ~OnnxModule();
	void ChangeDevice(Device _dev);

	static std::vector<std::wstring> CutLens(const std::wstring& input);

	[[nodiscard]] long GetSamplingRate() const
	{
		return _samplingRate;
	}

	template <typename T = float>
	static void LinearCombination(std::vector<T>& _data, T Value = T(1.0))
	{
		if(_data.empty())
		{
			_data = std::vector<T>(1, Value);
			return;
		}
		T Sum = T(0.0);
		for(const auto& i : _data)
			Sum += i;
		if (Sum < T(0.0001))
		{
			_data = std::vector<T>(_data.size(), T(0.0));
			_data[0] = Value;
			return;
		}
		Sum *= T(Value);
		for (auto& i : _data)
			i /= Sum;
	}
protected:
	Ort::Env* env = nullptr;
	Ort::SessionOptions* session_options = nullptr;
	Ort::MemoryInfo* memory_info = nullptr;

	modelType _modelType = modelType::SoVits;
	Device device_ = Device::CPU;

	long _samplingRate = 22050;

	callback _callback;

	static constexpr long MaxPath = 8000l;
	std::wstring _outputPath = GetCurrentFolder() + L"\\outputs";
};
 */

class SingingVoiceConversion : public MoeVoiceStudioModule
{
public:
	SingingVoiceConversion(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_ = 0);

	/**
	 * \brief 输入路径推理
	 * \param _Paths 路径，多个路径使用换行符隔开
	 * \param _InferParams 推理参数
	 * \param _SlicerSettings 切片机配置
	 * \return 输出路径
	 */
	[[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Paths,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
		const InferTools::SlicerSettings& _SlicerSettings) const override;

	/**
	 * \brief 推理一个音频
	 * \param _Slice 音频数据
	 * \param _InferParams 推理参数
	 * \return 推理结果（PCM signed-int16 单声道）
	 */
	[[nodiscard]] virtual std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice,
	                                                          const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const;

	/**
	 * \brief 推理一个音频（使用引用）
	 * \param _Slice 音频数据引用
	 * \param _InferParams 推理参数
	 * \param _Process 推理进度
	 * \return 推理结果（PCM signed-int16 单声道）
	 */
	[[nodiscard]] virtual std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process) const;

	/**
	 * \brief 推理一个音频（使用PCM）
	 * \param PCMData 输入的PCM数据（signed-int16 单声道）
	 * \param srcSr 输入PCM的采样率
	 * \param _InferParams 推理参数
	 * \return 推理结果（PCM signed-int16 单声道）
	 */
	[[nodiscard]] virtual std::vector<int16_t> InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const;

	//提取音量
	[[nodiscard]] static std::vector<float> ExtractVolume(const std::vector<int16_t>& OrgAudio, int hop_size);

	//提取音量
	[[nodiscard]] std::vector<float> ExtractVolume(const std::vector<double>& OrgAudio) const;

	//获取HopSize
	[[nodiscard]] int GetHopSize() const
	{
		return HopSize;
	}

	//获取HiddenUnitKDims
	[[nodiscard]] int64_t GetHiddenUnitKDims() const
	{
		return HiddenUnitKDims;
	}

	//获取角色数量
	[[nodiscard]] int64_t GetSpeakerCount() const
	{
		return SpeakerCount;
	}

	//获取角色混合启用状态
	[[nodiscard]] bool CharaMixEnabled() const
	{
		return EnableCharaMix;
	}

	/**
	 * \brief 切片一个音频
	 * \param input 输入的PCM数据（signed-int16 单声道）
	 * \param _slice_pos 切片位置（单位为采样）
	 * \param _slicer 切片机设置
	 * \return 音频数据
	 */
	LibSvcApi [[nodiscard]] static MoeVSProjectSpace::MoeVoiceStudioSvcData GetAudioSlice(const std::vector<int16_t>& input,
	                                                                      const std::vector<size_t>& _slice_pos,
	                                                                      const InferTools::SlicerSettings& _slicer);

	/**
	 * \brief 预处理音频数据
	 * \param input 完成切片的音频数据
	 * \param __SamplingRate 采样率
	 * \param __HopSize HopSize
	 * \param _f0_method F0算法
	 */
	LibSvcApi static void PreProcessAudio(MoeVSProjectSpace::MoeVoiceStudioSvcData& input,
	                            int __SamplingRate = 48000, int __HopSize = 512, const std::wstring& _f0_method = L"Dio");

	~SingingVoiceConversion() override;

protected:
	MoeVSTensorPreprocess::TensorExtractor _TensorExtractor;

	Ort::Session* hubert = nullptr;

	int HopSize = 320;
	int64_t HiddenUnitKDims = 256;
	int64_t SpeakerCount = 1;
	bool EnableCharaMix = false;
	bool EnableVolume = false;

	MoeVoiceStudioCluster::MoeVSCluster Cluster;

	int64_t ClusterCenterSize = 10000;
	bool EnableCluster = false;
	bool EnableIndex = false;

	const std::vector<const char*> hubertOutput = { "embed" };
	const std::vector<const char*> hubertInput = { "source" };
};

MoeVoiceStudioCoreEnd