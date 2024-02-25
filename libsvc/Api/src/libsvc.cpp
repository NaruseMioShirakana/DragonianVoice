#include "../header/libsvc.h"
#include <queue>
#include "../../header/InferTools/Stft/stft.hpp"
namespace libsvccore
{
	std::unordered_map<std::wstring, DiffusionSvc*> DiffusionSvcSessions;
	std::unordered_map<std::wstring, VitsSvc*> VitsSvcSessions;
	std::deque<std::wstring> ErrorMessage;
	std::wstring NoneError;
	size_t MaxErrorCount = 10;
	std::unordered_map<std::wstring, DlCodecStft::Mel*> MelOperators;
	Ort::MemoryInfo* vocoder_memory_info = nullptr;

	void RaiseError(const std::wstring& _Msg)
	{
		ErrorMessage.emplace_back(_Msg);
		logger.log(_Msg);
		if (ErrorMessage.size() > MaxErrorCount)
			ErrorMessage.pop_front();
	}

	void UnloadVitsSvcSession(const std::wstring& _Name)
	{
		const auto ModelPair = VitsSvcSessions.find(_Name);
		if (ModelPair != VitsSvcSessions.end())
		{
			delete ModelPair->second;
			VitsSvcSessions.erase(ModelPair);
		}
	}

	void UnloadDiffusionSvcSession(const std::wstring& _Name)
	{
		const auto ModelPair = DiffusionSvcSessions.find(_Name);
		if (ModelPair != DiffusionSvcSessions.end())
		{
			delete ModelPair->second;
			DiffusionSvcSessions.erase(ModelPair);
		}
	}

	int LoadDiffusionSvcSession(const Config& _Config, const std::wstring& _Name, const ProgressCallback& _ProgressCallback, ExecutionProvider ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_)
	{
		const auto ModelPair = DiffusionSvcSessions.find(_Name);
		if(ModelPair != DiffusionSvcSessions.end())
			delete ModelPair->second;
		try
		{
			DiffusionSvcSessions[_Name] = new MoeVoiceStudioCore::DiffusionSvc(
				_Config, _ProgressCallback,
				ExecutionProvider_,
				DeviceID_, ThreadCount_
			);
		}
		catch (std::exception& e)
		{
			RaiseError(to_wide_string(e.what()));
			UnloadDiffusionSvcSession(_Name);
			return 1;
		}
		return 0;
	}

	int LoadVitsSvcSession(const Config& _Config, const std::wstring& _Name, const ProgressCallback& _ProgressCallback, ExecutionProvider ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_)
	{
		const auto ModelPair = VitsSvcSessions.find(_Name);
		if (ModelPair != VitsSvcSessions.end())
			delete ModelPair->second;
		try
		{
			VitsSvcSessions[_Name] = new MoeVoiceStudioCore::VitsSvc(
				_Config, _ProgressCallback,
				ExecutionProvider_,
				DeviceID_, ThreadCount_
			);
		}
		catch (std::exception& e)
		{
			RaiseError(to_wide_string(e.what()));
			UnloadVitsSvcSession(_Name);
			return 1;
		}
		return 0;
	}

	void SetMaxErrorCount(size_t Count)
	{
		MaxErrorCount = Count;
	}

	std::wstring& GetLastError(size_t Index)
	{
		if (Index < MaxErrorCount)
			return ErrorMessage.at(MaxErrorCount - Index - 1);
		return NoneError;
	}

	int LoadModel(ModelType _T, const Config& _Config, const std::wstring& _Name, const ProgressCallback& _ProgressCallback, ExecutionProvider ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_)
	{
		if (_T == ModelType::Vits)
			return LoadVitsSvcSession(_Config, _Name, _ProgressCallback, ExecutionProvider_, DeviceID_, ThreadCount_);
		if (_T == ModelType::Diffusion)
			return LoadDiffusionSvcSession(_Config, _Name, _ProgressCallback, ExecutionProvider_, DeviceID_, ThreadCount_);
		return 1;
	}

	void UnloadModel(ModelType _T, const std::wstring& _Name)
	{
		if (_T == ModelType::Diffusion)
			UnloadDiffusionSvcSession(_Name);
		else if (_T == ModelType::Vits)
			UnloadVitsSvcSession(_Name);
	}

	void SafeFree(void** _Ptr)
	{
		if (*_Ptr)
			free(*_Ptr);
		*_Ptr = nullptr;
	}

	void SafeAlloc(void** _Ptr, size_t _Size)
	{
		*_Ptr = malloc(_Size);
	}

	void SliceAudio(void* _Output, const std::vector<int16_t>& _Audio, const InferTools::SlicerSettings& _Setting)
	{
		if (!_Output)
			return;
		const auto _Ptr = static_cast<std::vector<size_t>*>(_Output);
		*_Ptr = InferTools::SliceAudio(_Audio, _Setting);
	}

	void Preprocess(void* _Output, const std::vector<int16_t>& _Audio, const std::vector<size_t>& _SlicePos, const InferTools::SlicerSettings& _Setting, int _SamplingRate, int _HopSize, const std::wstring& _F0Method)
	{
		if (!_Output)
			return;
		const auto _Ptr = static_cast<Slices*>(_Output);
		*_Ptr = MoeVoiceStudioCore::SingingVoiceConversion::GetAudioSlice(_Audio, _SlicePos, _Setting);
		MoeVoiceStudioCore::SingingVoiceConversion::PreProcessAudio(*_Ptr, _SamplingRate, _HopSize, _F0Method);
	}

	int InferSlice(void* _Output, ModelType _T, const std::wstring& _Name, const SingleSlice& _Slice, const Params& _InferParams, size_t& _Process)
	{
		const MoeVoiceStudioCore::SingingVoiceConversion* _Model = nullptr;
		if (_T == ModelType::Vits && VitsSvcSessions.find(_Name) != VitsSvcSessions.end())
			_Model = dynamic_cast<MoeVoiceStudioCore::SingingVoiceConversion*>(VitsSvcSessions.at(_Name));
		else if (_T == ModelType::Diffusion && DiffusionSvcSessions.find(_Name) != DiffusionSvcSessions.end())
		{
			_Model = dynamic_cast<MoeVoiceStudioCore::SingingVoiceConversion*>(DiffusionSvcSessions.at(_Name));
			if(!MoeVSModuleManager::VocoderEnabled())
			{
				RaiseError(L"Vocoder Not Exists");
				return 1;
			}
		}
		if(!_Model)
		{
			RaiseError(L"Model Not Exists");
			return 1;
		}
		const auto _Ptr = static_cast<std::vector<int16_t>*>(_Output);
		try
		{
			*_Ptr = _Model->SliceInference(_Slice, _InferParams, _Process);
		}
		catch (std::exception& e)
		{
			RaiseError(to_wide_string(e.what()));
			return 1;
		}
		return 0;
	}

	int ShallowDiffusionInference(void* _Output, const std::wstring& _Name, const std::vector<float>& _16KAudioHubert, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, const std::pair<std::vector<float>, int64_t>& _Mel, const std::vector<float>& _SrcF0, const std::vector<float>& _SrcVolume, const std::vector<std::vector<float>>& _SrcSpeakerMap, size_t& Process, int64_t SrcSize)
	{
		if (!MoeVSModuleManager::VocoderEnabled())
		{
			RaiseError(L"Vocoder Not Exists");
			return 1;
		}
		if(DiffusionSvcSessions.find(_Name) != DiffusionSvcSessions.end())
		{
			const auto _Ptr = static_cast<std::vector<int16_t>*>(_Output);
			try
			{
				auto _Hubert = _16KAudioHubert;
				auto _MelCopy = _Mel;
				*_Ptr = DiffusionSvcSessions.at(_Name)->ShallowDiffusionInference(_Hubert, _InferParams, _MelCopy, _SrcF0, _SrcVolume, _SrcSpeakerMap, Process, SrcSize);
			}
			catch (std::exception& e)
			{
				RaiseError(to_wide_string(e.what()));
				return 1;
			}
			return 0;
		}
		RaiseError(L"Model Not Exists");
		return 1;
	}

	void Stft(void* _Output, const std::vector<double>& _NormalizedAudio, int _SamplingRate, int _Hopsize, int _MelBins)
	{
		const auto _Ptr = static_cast<std::pair<std::vector<float>, int64_t>*>(_Output);
		const std::wstring _Name = L"S" + std::to_wstring(_SamplingRate) + L"H" + std::to_wstring(_Hopsize) + L"M" + std::to_wstring(_MelBins);
		if (MelOperators.find(_Name) == MelOperators.end())
			MelOperators[_Name] = new DlCodecStft::Mel(_Hopsize * 4, _Hopsize, _SamplingRate, _MelBins);
		*_Ptr = MelOperators.at(_Name)->operator()(_NormalizedAudio);
	}

	void EmptyStftCache()
	{
		for (const auto& i : MelOperators)
			delete i.second;
		MelOperators.clear();
	}

	void LoadVocoder(const std::wstring& VocoderPath)
	{
		MoeVoiceStudioCore::LoadVocoderModel(VocoderPath);
	}

	int VocoderEnhance(void* _Output, const std::vector<float>& Mel, const std::vector<float>& F0, size_t MelSize, long VocoderMelBins)
	{
		if(!MoeVoiceStudioCore::VocoderEnabled())
		{
			RaiseError(L"Vocoder Not Exists");
			return 1;
		}
		const auto _Ptr = static_cast<std::vector<int16_t>*>(_Output);
		auto Rf0 = F0;
		auto MelTemp = Mel;
		if (Rf0.size() != MelSize)
			Rf0 = InferTools::InterpFunc(Rf0, (long)Rf0.size(), (long)MelSize);
		*_Ptr = MoeVoiceStudioCore::VocoderInfer(
			MelTemp,
			Rf0,
			VocoderMelBins,
			(int64_t)MelSize,
			vocoder_memory_info
		);
		return 0;
	}

	void Init()
	{
		MoeVSModuleManager::MoeVoiceStudioCoreInitSetup();
		vocoder_memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
		moevsenv::UseSingleOrtApiEnv(true);
	}

	void SetGlobalEnv(unsigned ThreadCount, unsigned DeviceID, unsigned Provider)
	{
		moevsenv::GetGlobalMoeVSEnv().Load(ThreadCount, DeviceID, Provider);
	}
}
