#pragma once
#include "../../Modules/header/Modules.hpp"

namespace libsvccore
{
	using Config = MoeVoiceStudioCore::Hparams;
	using VitsSvc = MoeVoiceStudioCore::VitsSvc;
	using DiffusionSvc = MoeVoiceStudioCore::DiffusionSvc;
	using ClusterBase = MoeVoiceStudioCluster::MoeVoiceStudioBaseCluster;
	using TensorExtractorBase = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor;
	using ProgressCallback = MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback;
	using ExecutionProvider = MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders;
	using Slices = MoeVSProjectSpace::MoeVoiceStudioSvcData;
	using SingleSlice = MoeVSProjectSpace::MoeVoiceStudioSvcSlice;
	using Params = MoeVSProjectSpace::MoeVSSvcParams;
	enum class ModelType { Vits, Diffusion };

	LibSvcApi void SafeAlloc(void** _Ptr, size_t _Size);

	LibSvcApi void SafeFree(void** _Ptr);

	LibSvcApi void SliceAudio(void* _Output, const std::vector<int16_t>& _Audio, const InferTools::SlicerSettings& _Setting);

	LibSvcApi void Preprocess(void* _Output, const std::vector<int16_t>& _Audio, const std::vector<size_t>& _SlicePos, const InferTools::SlicerSettings& _Setting, int _SamplingRate = 48000, int _HopSize = 512, const std::wstring& _F0Method = L"Dio");

	LibSvcApi int LoadModel(ModelType _T, const Config& _Config, const std::wstring& _Name, const ProgressCallback& _ProgressCallback,
		ExecutionProvider ExecutionProvider_ = ExecutionProvider::CPU,
		unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

	LibSvcApi void UnloadModel(ModelType _T, const std::wstring& _Name);

	LibSvcApi int InferSlice(void* _Output, ModelType _T, const std::wstring& _Name, const SingleSlice& _Slice, const Params& _InferParams, size_t& _Process);

	LibSvcApi int ShallowDiffusionInference(
		void* _Output, const std::wstring& _Name,
		const std::vector<float>& _16KAudioHubert,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
		const std::pair<std::vector<float>, int64_t>& _Mel,
		const std::vector<float>& _SrcF0,
		const std::vector<float>& _SrcVolume,
		const std::vector<std::vector<float>>& _SrcSpeakerMap,
		size_t& Process,
		int64_t SrcSize
	);
	
	LibSvcApi void Stft(void* _Output, const std::vector<double>& _NormalizedAudio, int _SamplingRate, int _Hopsize, int _MelBins);

	LibSvcApi int VocoderEnhance(void* _Output, std::vector<float>& Mel, const std::vector<float>& F0, size_t MelSize, long VocoderMelBins);

	/// <summary>
	/// 设置Error队列的最大长度（缓存Error信息）
	/// </summary>
	/// <param name="Count">长度</param>
	/// <returns></returns>
	LibSvcApi void SetMaxErrorCount(size_t Count);

	/// <summary>
	/// 获取Error队列的倒数Index个
	/// </summary>
	/// <param name="Index">倒序索引</param>
	/// <returns></returns>
	LibSvcApi std::wstring& GetLastError(size_t Index);

	/// <summary>
	/// 清除Stft缓存
	/// </summary>
	/// <returns></returns>
	LibSvcApi void EmptyStftCache();

	/// <summary>
	/// 加载声码器模型
	/// </summary>
	/// <param name="VocoderPath">声码器模型路径</param>
	/// <returns></returns>
	LibSvcApi void LoadVocoder(const std::wstring& VocoderPath);

	/// <summary>
	/// 初始化函数（使用前必须调用）
	/// </summary>
	/// <returns></returns>
	LibSvcApi void Init();
}

namespace libsvc
{
	using libsvccore::Config;
	using libsvccore::VitsSvc;
	using libsvccore::DiffusionSvc;
	using libsvccore::ClusterBase;
	using libsvccore::TensorExtractorBase;
	using libsvccore::ProgressCallback;
	using libsvccore::ExecutionProvider;
	using libsvccore::ModelType;
	using libsvccore::Slices;
	using libsvccore::SingleSlice;
	using libsvccore::Params;
	using MelContainer = std::pair<std::vector<float>, int64_t>;

	using libsvccore::SetMaxErrorCount;
	using libsvccore::GetLastError;
	using libsvccore::EmptyStftCache;
	using libsvccore::LoadVocoder;
	using libsvccore::Init;

	/// <summary>
	/// 卸载一个模型
	/// </summary>
	/// <param name="_T">模型类别</param>
	/// <param name="_Name">模型ID</param>
	/// <returns></returns>
	inline void UnloadModel(
		ModelType _T,
		const std::wstring& _Name
	)
	{
		libsvccore::UnloadModel(_T, _Name);
	}

	/// <summary>
	/// 加载一个模型
	/// </summary>
	/// <param name="_T">模型类别</param>
	/// <param name="_Config">配置项目</param>
	/// <param name="_Name">模型ID</param>
	/// <param name="_ProgressCallback">进度条回调函数</param>
	/// <param name="ExecutionProvider_">ExecutionProvider</param>
	/// <param name="DeviceID_">GPUID</param>
	/// <param name="ThreadCount_">线程数</param>
	/// <returns>成功加载：0， 失败：非0</returns>
	inline int LoadModel(
		ModelType _T,
		const Config& _Config,
		const std::wstring& _Name,
		const ProgressCallback& _ProgressCallback,
		ExecutionProvider ExecutionProvider_ = ExecutionProvider::CPU,
		unsigned DeviceID_ = 0,
		unsigned ThreadCount_ = 0
	)
	{
		return libsvccore::LoadModel(_T, _Config, _Name, _ProgressCallback, ExecutionProvider_, DeviceID_, ThreadCount_);
	}

	/**
	 * \brief 切片音频
	 * \param _Audio 输入音频（必须是PCM-Signed-Int16 单声道）
	 * \param _Setting 切片机配置
	 * \return 切片Pos
	 */
	inline std::vector<size_t> SliceAudio(
		const std::vector<int16_t>& _Audio,
		const InferTools::SlicerSettings& _Setting
	)
	{
		void* _Ptr = nullptr;
		libsvccore::SafeAlloc(&_Ptr, sizeof(std::vector<size_t>));
		if (!_Ptr)
			return {};
		libsvccore::SliceAudio(_Ptr, _Audio, _Setting);
		std::vector temp = *static_cast<std::vector<size_t>*>(_Ptr);
		libsvccore::SafeFree(&_Ptr);
		return temp;
	}

	/**
	 * \brief 预处理切片好的音频
	 * \param _Audio 输入音频
	 * \param _SlicePos 切片Pos
	 * \param _Setting 切片机配置
	 * \param _SamplingRate 采样率
	 * \param _HopSize Hopsize
	 * \param _F0Method F0算法
	 * \return 预处理后的数据（可以自己进一步处理或者直接送进推理函数推理）
	 */
	inline Slices PreprocessSlices(
		const std::vector<int16_t>& _Audio,
		const std::vector<size_t>& _SlicePos,
		const InferTools::SlicerSettings& _Setting,
		int _SamplingRate = 48000,
		int _HopSize = 512,
		const std::wstring& _F0Method = L"Dio"
	)
	{
		void* _Ptr = nullptr;
		libsvccore::SafeAlloc(&_Ptr, sizeof(Slices));
		if (!_Ptr)
			return {};
		libsvccore::Preprocess(_Ptr, _Audio, _SlicePos, _Setting, _SamplingRate, _HopSize, _F0Method);
		Slices temp = *static_cast<Slices*>(_Ptr);
		libsvccore::SafeFree(&_Ptr);
		return temp;
	}

	/**
	 * \brief 短时傅里叶变换（Mel）
	 * \param _NormalizedAudio 归一化音频PCM数据
	 * \param _SamplingRate 采样率
	 * \param _Hopsize HopSize
	 * \param _MelBins MelBins
	 * \return Mel
	 */
	inline MelContainer Stft(
		const std::vector<double>& _NormalizedAudio,
		int _SamplingRate,
		int _Hopsize,
		int _MelBins
	)
	{
		void* _Ptr = nullptr;
		libsvccore::SafeAlloc(&_Ptr, sizeof(MelContainer));
		if (!_Ptr)
			return {};
		libsvccore::Stft(_Ptr, _NormalizedAudio, _SamplingRate, _Hopsize, _MelBins);
		MelContainer temp = *static_cast<MelContainer*>(_Ptr);
		return temp;
	}

	/**
	 * \brief 推理一个切片
	 * \param _T 模型类型
	 * \param _Name 模型ID
	 * \param _Slice 切片数据
	 * \param _InferParams 推理参数
	 * \param _Process 总进度
	 * \return 音频PCM数据
	 */
	inline std::vector<int16_t> InferSlice(
		ModelType _T,
		const std::wstring& _Name,
		const SingleSlice& _Slice,
		const Params& _InferParams,
		size_t& _Process
	)
	{
		void* _Ptr = nullptr;
		libsvccore::SafeAlloc(&_Ptr, sizeof(std::vector<int16_t>));
		if (!_Ptr)
			return {};
		if(!libsvccore::InferSlice(_Ptr, _T, _Name, _Slice, _InferParams, _Process))
		{
			std::vector<int16_t> temp = *static_cast<std::vector<int16_t>*>(_Ptr);
			return temp;
		}
		return {};
	}

	/**
	 * \brief 浅扩散推理
	 * \param _Name Diffusion模型ID
	 * \param _16KAudioHubert 16000采样率的原始音频（或前级音频）
	 * \param _InferParams 推理参数
	 * \param _Mel Mel
	 * \param _SrcF0 基频序列
	 * \param _SrcVolume 音量序列
	 * \param _SrcSpeakerMap 说话人比例序列
	 * \param Process 进度序列
	 * \param SrcSize 原始音频长度（切片机切片出来的长度）
	 * \return 音频PCM数据
	 */
	inline std::vector<int16_t> ShallowDiffusionInference(
		const std::wstring& _Name,
		const std::vector<float>& _16KAudioHubert,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
		const MelContainer& _Mel,
		const std::vector<float>& _SrcF0,
		const std::vector<float>& _SrcVolume,
		const std::vector<std::vector<float>>& _SrcSpeakerMap,
		size_t& Process,
		int64_t SrcSize
	)
	{
		void* _Ptr = nullptr;
		libsvccore::SafeAlloc(&_Ptr, sizeof(std::vector<int16_t>));
		if (!_Ptr)
			return {};
		if(!libsvccore::ShallowDiffusionInference(_Ptr, _Name, _16KAudioHubert, _InferParams, _Mel, _SrcF0, _SrcVolume, _SrcSpeakerMap, Process, SrcSize))
		{
			std::vector<int16_t> temp = *static_cast<std::vector<int16_t>*>(_Ptr);
			return temp;
		}
		return {};
	}

	/**
	 * \brief 声码器增强（声码器推理）
	 * \param Mel Mel
	 * \param F0 基频序列
	 * \param MelSize Mel的帧数
	 * \param VocoderMelBins 声码器的MelBins
	 * \return PCM数据
	 */
	inline std::vector<int16_t> VocoderEnhance(std::vector<float>& Mel, const std::vector<float>& F0, size_t MelSize, long VocoderMelBins)
	{
		void* _Ptr = nullptr;
		libsvccore::SafeAlloc(&_Ptr, sizeof(std::vector<int16_t>));
		if (!_Ptr)
			return {};
		if(!libsvccore::VocoderEnhance(_Ptr, Mel, F0, MelSize, VocoderMelBins))
		{
			std::vector<int16_t> temp = *static_cast<std::vector<int16_t>*>(_Ptr);
			return temp;
		}
		return {};
	}
}