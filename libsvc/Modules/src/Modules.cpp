#include "../header/Modules.hpp"
#include "../header/InferTools/F0Extractor/DioF0Extractor.hpp"
#include "../header/InferTools/F0Extractor/F0ExtractorManager.hpp"
#include "../header/InferTools/TensorExtractor/MoeVSCoreTensorExtractor.hpp"
#include "../header/InferTools/Cluster/MoeVSClusterManager.hpp"
#include "../header/InferTools/Cluster/MoeVSKmeansCluster.hpp"
#include "../header/InferTools/F0Extractor/HarvestF0Extractor.hpp"
#include "../header/InferTools/Sampler/MoeVSSamplerManager.hpp"
#include "../header/InferTools/Sampler/MoeVSSamplers.hpp"
#include "../header/InferTools/F0Extractor/NetF0Predictors.hpp"
#include "../header/InferTools/Stft/stft.hpp"
#ifdef MoeVoiceStudioIndexCluster
#ifdef max
#undef max
#endif
#include "../header/InferTools/Cluster/MoeVSIndexCluster.hpp"
#endif

#define MoeVSRegisterF0Constructor(__RegisterName, __ClassName) MoeVSF0Extractor::RegisterF0Extractor(__RegisterName,   \
	[](int32_t sampling_rate, int32_t hop_size,																			\
	int32_t n_f0_bins, double max_f0, double min_f0)																	\
	-> MoeVSF0Extractor::F0Extractor																					\
	{																													\
		return new MoeVSF0Extractor::__ClassName(sampling_rate, hop_size, n_f0_bins, max_f0, min_f0);					\
	})

#define MoeVSRegisterTensorConstructor(__RegisterName, __ClassName) MoeVSTensorPreprocess::RegisterTensorExtractor(__RegisterName,\
	[](uint64_t _srcsr, uint64_t _sr, uint64_t _hop,													        \
		bool _smix, bool _volume, uint64_t _hidden_size,													    \
		uint64_t _nspeaker,																	                    \
		const MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Others& _other)                             \
		->MoeVSTensorPreprocess::TensorExtractor													            \
	{																										    \
		return new MoeVSTensorPreprocess::__ClassName(_srcsr, _sr, _hop, _smix, _volume,						\
			_hidden_size, _nspeaker, _other);													                \
	})

#define MoeVSRegisterCluster(__RegisterName, __ClassName) MoeVoiceStudioCluster::RegisterMoeVSCluster(__RegisterName,\
	[](const std::wstring& _path, size_t hidden_size, size_t KmeansLen)												 \
		->MoeVoiceStudioCluster::MoeVSCluster																		 \
	{																												 \
		return new MoeVoiceStudioCluster::__ClassName(_path, hidden_size, KmeansLen);								 \
	})

#define MoeVSRegisterSampler(__RegisterName, __ClassName) MoeVSSampler::RegisterMoeVSSampler(__RegisterName,		 \
	[](Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins,								 \
		const MoeVSSampler::MoeVSBaseSampler::ProgressCallback& _ProgressCallback,								 	 \
		Ort::MemoryInfo* memory) -> MoeVSSampler::MoeVSSampler														 \
	{																												 \
		return new MoeVSSampler::__ClassName(alpha, dfn, pred, Mel_Bins, _ProgressCallback, memory);				 \
	})

namespace MoeVSModuleManager
{
	bool MoeVoiceStudioCoreInitStat = false;

	MoeVoiceStudioCore::VitsSvc* GlobalVitsSvcModel = nullptr;

	MoeVoiceStudioCore::DiffusionSvc* GlobalDiffusionSvcModel = nullptr;

	MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback ProgessBar;

	int64_t SpeakerCount = 0;
	int64_t SamplingRate = 32000;
	int32_t VocoderHopSize = 512;
	int32_t VocoderMelBins = 128;
	int64_t VitsSamplingRate = 32000;

	int64_t& GetSamplingRate()
	{
		return SamplingRate;
	}

	int64_t& GetSpeakerCount()
	{
		return SpeakerCount;
	}

	int32_t& GetVocoderHopSize()
	{
		return VocoderHopSize;
	}

	int32_t& GetVocoderMelBins()
	{
		return VocoderMelBins;
	}

	void MoeVoiceStudioCoreInitSetup()
	{
		if (MoeVoiceStudioCoreInitStat)
			return;
		MoeVSRegisterF0Constructor(L"Dio", DioF0Extractor);
		MoeVSRegisterF0Constructor(L"Harvest", HarvestF0Extractor);
		MoeVSRegisterF0Constructor(L"RMVPE", RMVPEF0Extractor);
		MoeVSRegisterF0Constructor(L"FCPE", MELPEF0Extractor);
		MoeVSRegisterTensorConstructor(L"SoVits2.0", SoVits2TensorExtractor);
		MoeVSRegisterTensorConstructor(L"SoVits3.0", SoVits3TensorExtractor);
		MoeVSRegisterTensorConstructor(L"SoVits4.0", SoVits4TensorExtractor);
		MoeVSRegisterTensorConstructor(L"SoVits4.0-DDSP", SoVits4DDSPTensorExtractor);
		MoeVSRegisterTensorConstructor(L"RVC", RVCTensorExtractor);
		MoeVSRegisterTensorConstructor(L"DiffSvc", DiffSvcTensorExtractor);
		MoeVSRegisterTensorConstructor(L"DiffusionSvc", DiffusionSvcTensorExtractor);
		MoeVSRegisterCluster(L"KMeans", KMeansCluster);
#ifdef MoeVoiceStudioIndexCluster
		MoeVSRegisterCluster(L"Index", IndexCluster);
#endif
		MoeVSRegisterSampler(L"Pndm", PndmSampler);
		MoeVSRegisterSampler(L"DDim", DDimSampler);
		MoeVoiceStudioCoreInitStat = true;
	}

	MoeVoiceStudioCore::VitsSvc* GetVitsSvcModel()
	{
		return GlobalVitsSvcModel;
	}

	MoeVoiceStudioCore::DiffusionSvc* GetDiffusionSvcModel()
	{
		return GlobalDiffusionSvcModel;
	}

	void UnloadVitsSvcModel()
	{
		delete GlobalVitsSvcModel;
		GlobalVitsSvcModel = nullptr;
	}

	void UnloadDiffusionSvcModel()
	{
		delete GlobalDiffusionSvcModel;
		GlobalDiffusionSvcModel = nullptr;
	}

	void LoadVitsSvcModel(const MJson& Config,
	                  const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
	                  int ProviderID, int NumThread, int DeviceID)
	{
		ProgessBar = Callback;
		UnloadVitsSvcModel();
		if (Config["Type"].GetString() == "DiffSvc")
			LibDLVoiceCodecThrow("Trying To Load Diffusion Model As VitsSvc Model!")

		GlobalVitsSvcModel = new MoeVoiceStudioCore::VitsSvc(
			Config, Callback,
			MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
			DeviceID, NumThread
		);

		VitsSamplingRate = GlobalVitsSvcModel->GetSamplingRate();
		SamplingRate = VitsSamplingRate;
	}

	void LoadDiffusionSvcModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID)
	{
		ProgessBar = Callback;
		UnloadDiffusionSvcModel();
		if (Config["Type"].GetString() == "DiffSvc")
			GlobalDiffusionSvcModel = new MoeVoiceStudioCore::DiffusionSvc(
				Config, Callback,
				MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				DeviceID, NumThread
			);
		else
			LibDLVoiceCodecThrow("Trying To Load VitsSvc Model As Diffusion Model!")

		if(VocoderEnabled())
			if (!GetVitsSvcModel() || (!GlobalDiffusionSvcModel->OldVersion() && GlobalDiffusionSvcModel->GetDiffSvcVer() == L"DiffusionSvc"))
				SamplingRate = GlobalDiffusionSvcModel->GetSamplingRate();
	}

	void LoadVocoderModel(const std::wstring& VocoderPath)
	{
		MoeVoiceStudioCore::LoadVocoderModel(VocoderPath);
	}

	void UnloadVocoderModel()
	{
		MoeVoiceStudioCore::UnLoadVocoderModel();
	}

	bool VocoderEnabled()
	{
		return MoeVoiceStudioCore::VocoderEnabled();
	}

	std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice,
		const MoeVSProjectSpace::MoeVSSvcParams& _InferParams)
	{
		const bool DiffusionModelEnabled = GlobalDiffusionSvcModel && VocoderEnabled();
		std::vector<int16_t> RtnAudio;
		size_t TotalAudioSize = 0;
		for (const auto& data_size : _Slice.Slices)
			TotalAudioSize += data_size.OrgLen;
		RtnAudio.reserve(size_t(double(TotalAudioSize) * 1.5));

		//VitsSteps
		int64_t GlobalSteps = 0;
		if (GlobalVitsSvcModel)
			GlobalSteps = 1;

		//DiffusionSteps
		auto SkipDiffusionStep = (int64_t)_InferParams.Pndm;
		auto DiffusionTotalStep = (int64_t)_InferParams.Step;
		
		if (DiffusionModelEnabled && DiffusionTotalStep > GlobalDiffusionSvcModel->GetMaxStep()) 
			DiffusionTotalStep = GlobalDiffusionSvcModel->GetMaxStep();
		if (SkipDiffusionStep >= DiffusionTotalStep) 
			SkipDiffusionStep = DiffusionTotalStep / 5;
		if (SkipDiffusionStep == 0) 
			SkipDiffusionStep = 1;
		const auto RealDiffSteps = DiffusionTotalStep % SkipDiffusionStep ? DiffusionTotalStep / SkipDiffusionStep + 1 : DiffusionTotalStep / SkipDiffusionStep;
		if((DiffusionModelEnabled && !GlobalVitsSvcModel) || (DiffusionModelEnabled && ShallowDiffusionEnabled()))
		{
			if (GlobalDiffusionSvcModel->OldVersion())
				GlobalSteps += 1;
			else
				GlobalSteps += RealDiffSteps;
		}

		//TotalSteps
		const int64_t TotalSteps = GlobalSteps * int64_t(_Slice.Slices.size());
		size_t ProgressVal = 0;
		size_t SliceIndex = 0;
		ProgessBar(0, TotalSteps);
		for (const auto& CurSlice : _Slice.Slices)
		{
			const auto InferBeginTime = clock();
			const auto CurRtn = SliceInference(CurSlice, _InferParams, ProgressVal);
			RtnAudio.insert(RtnAudio.end(), CurRtn.data(), CurRtn.data() + CurRtn.size());
			if (CurSlice.IsNotMute)
				logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Segment[" + std::to_wstring(SliceIndex++) + L"] Finished! Segment Use Time: " + std::to_wstring(clock() - InferBeginTime) + L"ms, Segment Duration: " + std::to_wstring((size_t)CurSlice.OrgLen * 1000ull / 48000ull) + L"ms");
			else
			{
				if (DiffusionModelEnabled && !GlobalDiffusionSvcModel->OldVersion())
				{
					ProgressVal += RealDiffSteps;
					ProgessBar(ProgressVal, TotalSteps);
				}
				logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Jump Empty Segment[" + std::to_wstring(SliceIndex++) + L"]!");
			}
			if (DiffusionModelEnabled && GlobalDiffusionSvcModel->OldVersion())
				ProgessBar(++ProgressVal, TotalSteps);
			if (GlobalVitsSvcModel)
				ProgessBar(++ProgressVal, TotalSteps);
		}

		logger.log(L"[Inferring] \"" + _Slice.Path + L"\" Finished");

		return RtnAudio;
	}

	int CurStftSr = -1, CurHopSize = -1, CurMelBins = -1;

	DlCodecStft::Mel* MelOperator = nullptr;

	void ReloadMelOps(int SamplingRate_I64, int Hopsize_I64, int MelBins_I64)
	{
		if (CurStftSr != SamplingRate_I64 || CurHopSize != Hopsize_I64 || CurMelBins != MelBins_I64)
		{
			delete MelOperator;
			CurStftSr = SamplingRate_I64;
			CurHopSize = Hopsize_I64;
			CurMelBins = MelBins_I64;
			MelOperator = new DlCodecStft::Mel(Hopsize_I64 * 4, CurHopSize, CurStftSr, CurMelBins);
		}
	}

	std::vector<int16_t> Enhancer(std::vector<float>& Mel, const std::vector<float>& F0, size_t MelSize)
	{
		auto Rf0 = F0;
		if (Rf0.size() != MelSize)
			Rf0 = InferTools::InterpFunc(Rf0, (long)Rf0.size(), (long)MelSize);
		return MoeVoiceStudioCore::VocoderInfer(
			Mel,
			Rf0,
			VocoderMelBins,
			(int64_t)MelSize,
			GlobalVitsSvcModel->GetMemoryInfo()
		);
	}

	std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process)
	{
		const bool DiffusionModelEnabled = GlobalDiffusionSvcModel && VocoderEnabled();
		int64_t SamplingRate_I64 = VitsSamplingRate;
		SamplingRate = VitsSamplingRate;
		if ((DiffusionModelEnabled && !GlobalVitsSvcModel) || (_InferParams.UseShallowDiffusion && DiffusionModelEnabled && ShallowDiffusionEnabled()))
		{
			SamplingRate_I64 = GlobalDiffusionSvcModel->GetSamplingRate();
			SamplingRate = GlobalDiffusionSvcModel->GetSamplingRate();
		}
		if (!_Slice.IsNotMute)
			return { size_t(_Slice.OrgLen * SamplingRate_I64 / 48000), 0i16, std::allocator<int16_t>() };
		std::vector<int16_t> RtnAudio;
		RtnAudio.reserve(size_t(double(_Slice.Audio.size()) * 1.5));

		if (GlobalVitsSvcModel)
		{
			auto BgnTime = clock();
			RtnAudio = GlobalVitsSvcModel->SliceInference(_Slice, _InferParams, _Process);
			logger.log(("[Inference] Slice Vits Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
			if(_InferParams.UseShallowDiffusion && DiffusionModelEnabled && !GlobalDiffusionSvcModel->OldVersion() && GlobalDiffusionSvcModel->GetDiffSvcVer() == L"DiffusionSvc")
			{
				ReloadMelOps(
					(int)SamplingRate_I64,
					GlobalDiffusionSvcModel->GetHopSize(),
					(int)GlobalDiffusionSvcModel->GetMelBins()
				);

				const auto TempAudio = InferTools::InterpResample(RtnAudio, (long)VitsSamplingRate, CurStftSr, 32767.);
				auto Mel = MelOperator->operator()(TempAudio);
				auto& ShallData = MoeVoiceStudioCore::GetDataForShallowDiffusion();
				BgnTime = clock();
				RtnAudio = GlobalDiffusionSvcModel->ShallowDiffusionInference(
					ShallData._16KAudio, _InferParams, Mel,
					ShallData.NeedPadding ? ShallData.CUDAF0 : _Slice.F0,
					ShallData.NeedPadding ? ShallData.CUDAVolume : _Slice.Volume,
					ShallData.NeedPadding ? ShallData.CUDASpeaker : _Slice.Speaker,
					_Process,
					(int64_t)TempAudio.size()
				);
				logger.log(("[Inference] Slice Diffusion Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
			}
			if(_InferParams.UseShallowDiffusion && VocoderEnabled() && !GetDiffusionSvcModel())
			{
				ReloadMelOps(
					(int)SamplingRate_I64,
					VocoderHopSize,
					VocoderMelBins
				);
				const auto TempAudio = InferTools::InterpResample(RtnAudio, (long)SamplingRate_I64, (long)SamplingRate_I64, 32767.);
				auto Mel = MelOperator->operator()(TempAudio);
				const auto& ShallData = MoeVoiceStudioCore::GetDataForShallowDiffusion();
				const auto SrcSize = RtnAudio.size();
				RtnAudio = Enhancer(Mel.first, ShallData.NeedPadding ? ShallData.CUDAF0 : _Slice.F0, (size_t)Mel.second);
				RtnAudio.resize(SrcSize, 0);
			}
		}
		else if (DiffusionModelEnabled)
		{
			const auto BgnTime = clock();
			RtnAudio = GlobalDiffusionSvcModel->SliceInference(_Slice, _InferParams, _Process);
			logger.log(("[Inference] Slice Diffusion Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
		}
		else
			LibDLVoiceCodecThrow("You Must Load A Model To Inference!");

		return RtnAudio;
	}

	bool ShallowDiffusionEnabled()
	{
		const bool DiffusionModelEnabled = GlobalDiffusionSvcModel && VocoderEnabled();
		return DiffusionModelEnabled && !GlobalDiffusionSvcModel->OldVersion() && GlobalDiffusionSvcModel->GetDiffSvcVer() == L"DiffusionSvc";
	}
}