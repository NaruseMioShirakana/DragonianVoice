#pragma once
#include "ModelBase.hpp"
#include "../../Logger/MoeSSLogger.hpp"
#include "MJson.h"
#include "../../InferTools/inferTools.hpp"
#include "../../InferTools/TensorExtractor/TensorExtractorManager.hpp"
#include "../../InferTools/Cluster/MoeVSClusterManager.hpp"

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

	//输入多个路径批量推理
	[[nodiscard]] virtual std::vector<std::wstring> Inference(std::wstring& _Paths,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
		const InferTools::SlicerSettings& _SlicerSettings) const;

	//使用单个音频的结构体进行推理
	[[nodiscard]] virtual std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const;

	[[nodiscard]] virtual std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSliceRef& _Slice,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const;

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

	[[nodiscard]] bool CharaMixEnabled() const
	{
		return EnableCharaMix;
	}

	//切片音频
	[[nodiscard]] static MoeVSProjectSpace::MoeVSAudioSlice GetAudioSlice(const std::vector<int16_t>& input,
		const std::vector<size_t>& _slice_pos,
		const InferTools::SlicerSettings& _slicer);

	//预处理音频
	static void PreProcessAudio(MoeVSProjectSpace::MoeVSAudioSlice& input,
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

	Ort::AllocatorWithDefaultOptions allocator;
	const std::vector<const char*> hubertOutput = { "embed" };
	const std::vector<const char*> hubertInput = { "source" };
};

MoeVoiceStudioCoreEnd