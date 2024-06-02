/**
 * FileName: Modules.hpp
 * Note: MoeVoiceStudioCore组件管理
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
#include "Models/VitsSvc.hpp"
#include "Models/DiffSvc.hpp"
#include "Models/ReflowSvc.hpp"
#include "../framework.h"
#include "InferTools/Stft/stft.hpp"

namespace MoeVSModuleManager
{
	class UnionSvcModel
	{
	public:
		UnionSvcModel() = delete;
		LibSvcApi ~UnionSvcModel();
		LibSvcApi UnionSvcModel(const MJson& Config,
			const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
			int ProviderID, int NumThread, int DeviceID);

		LibSvcApi UnionSvcModel(const MoeVoiceStudioCore::Hparams& Config,
			const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
			int ProviderID, int NumThread, int DeviceID);

		LibSvcApi [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice,
			const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const;

		LibSvcApi [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process) const;

		LibSvcApi [[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Paths, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
			const InferTools::SlicerSettings& _SlicerSettings) const;

		LibSvcApi [[nodiscard]] std::vector<int16_t> InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const;

		LibSvcApi [[nodiscard]] std::vector<int16_t> ShallowDiffusionInference(
			std::vector<float>& _16KAudioHubert,
			const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
			std::pair<std::vector<float>, int64_t>& _Mel,
			const std::vector<float>& _SrcF0,
			const std::vector<float>& _SrcVolume,
			const std::vector<std::vector<float>>& _SrcSpeakerMap,
			size_t& Process,
			int64_t SrcSize
		) const;

		LibSvcApi MoeVoiceStudioCore::SingingVoiceConversion* GetPtr() const;

		LibSvcApi [[nodiscard]] int64_t GetMaxStep() const;

		LibSvcApi [[nodiscard]] bool OldVersion() const;

		LibSvcApi [[nodiscard]] const std::wstring& GetDiffSvcVer() const;

		LibSvcApi [[nodiscard]] int64_t GetMelBins() const;

		LibSvcApi [[nodiscard]] int GetHopSize() const;

		LibSvcApi [[nodiscard]] int64_t GetHiddenUnitKDims() const;

		LibSvcApi [[nodiscard]] int64_t GetSpeakerCount() const;

		LibSvcApi [[nodiscard]] bool CharaMixEnabled() const;

		LibSvcApi [[nodiscard]] long GetSamplingRate() const;

		LibSvcApi void NormMel(std::vector<float>& MelSpec) const;

		[[nodiscard]] bool IsDiffusion() const;
	private:
		MoeVoiceStudioCore::DiffusionSvc* Diffusion_ = nullptr;
		MoeVoiceStudioCore::ReflowSvc* Reflow_ = nullptr;
	};


	LibSvcApi int64_t& GetSpeakerCount();
	LibSvcApi int64_t& GetSamplingRate();
	LibSvcApi int32_t& GetVocoderHopSize();
	LibSvcApi int32_t& GetVocoderMelBins();
	/**
	 * \brief 初始化所有组件
	 */
	LibSvcApi void MoeVoiceStudioCoreInitSetup();

	/**
	 * \brief 获取当前VitsSvc模型
	 * \return 当前模型的指针
	 */
	LibSvcApi MoeVoiceStudioCore::VitsSvc* GetVitsSvcModel();

	/**
	 * \brief 获取当前UnionSvc模型
	 * \return 当前模型的指针
	 */
	LibSvcApi UnionSvcModel* GetUnionSvcModel();

	/**
	 * \brief 卸载模型
	 */
	LibSvcApi void UnloadVitsSvcModel();

	/**
	 * \brief 卸载模型
	 */
	LibSvcApi void UnloadUnionSvcModel();

	/**
	 * \brief 载入VitsSvc模型
	 * \param Config 一个MJson类的实例（配置文件的JSON）
	 * \param Callback 进度条回调函数
	 * \param ProviderID Provider在所有Provider中的ID（遵循Enum Class的定义）
	 * \param NumThread CPU推理时的线程数（最好设置高一点，GPU不支持的算子可能也会Fallback到CPU）
	 * \param DeviceID GPU设备ID
	 */
	LibSvcApi void LoadVitsSvcModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID);

	/**
	 * \brief 载入DiffusionSvc模型
	 * \param Config 一个MJson类的实例（配置文件的JSON）
	 * \param Callback 进度条回调函数
	 * \param ProviderID Provider在所有Provider中的ID（遵循Enum Class的定义）
	 * \param NumThread CPU推理时的线程数（最好设置高一点，GPU不支持的算子可能也会Fallback到CPU）
	 * \param DeviceID GPU设备ID
	 */
	LibSvcApi void LoadUnionSvcModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID);

	/**
	 * \brief 载入Vocoder模型
	 * \param VocoderPath Vocoder路径
	 */
	LibSvcApi void LoadVocoderModel(const std::wstring& VocoderPath);

	/**
	 * \brief 卸载Vocoder模型
	 */
	LibSvcApi void UnloadVocoderModel();

	/**
	 * \brief 检查Vocoder是否可用
	 * \return Vocoder状态
	 */
	LibSvcApi bool VocoderEnabled();

	/**
	 * \brief 推理多组数据
	 * \param _Slice 数据包
	 * \param _InferParams 参数
	 * \return 音频
	 */
	LibSvcApi std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice,
	                                    const MoeVSProjectSpace::MoeVSSvcParams& _InferParams);

	/**
	 * \brief 推理切片数据
	 * \param _Slice 切片数据
	 * \param _InferParams 参数
	 * \param _Process 进度条
	 * \return 音频
	 */
	LibSvcApi std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process);

	LibSvcApi std::vector<int16_t> Enhancer(std::vector<float>& Mel, const std::vector<float>& F0, size_t MelSize);

	LibSvcApi void ReloadMelOps(int SamplingRate_I64, int Hopsize_I64, int MelBins_I64);

	LibSvcApi DlCodecStft::Mel& GetMelOperator();

	LibSvcApi bool ShallowDiffusionEnabled();
}

namespace MoeVSRename
{
	using MoeVSVitsBasedSvc = MoeVoiceStudioCore::VitsSvc;
	using MoeVSDiffBasedSvc = MoeVoiceStudioCore::DiffusionSvc;
}

