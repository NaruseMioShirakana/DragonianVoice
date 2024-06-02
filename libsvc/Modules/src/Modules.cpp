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

#define MoeVSRegisterReflowSampler(__RegisterName, __ClassName) MoeVSSampler::RegisterMoeVSReflowSampler(__RegisterName,		 \
	[](Ort::Session* velocity, int64_t Mel_Bins,								 \
		const MoeVSSampler::MoeVSBaseSampler::ProgressCallback& _ProgressCallback,								 	 \
		Ort::MemoryInfo* memory) -> MoeVSSampler::MoeVSReflowSampler														 \
	{																												 \
		return new MoeVSSampler::__ClassName(velocity, Mel_Bins, _ProgressCallback, memory);				 \
	})

namespace MoeVSModuleManager
{
	MoeVoiceStudioCore::SingingVoiceConversion* UnionSvcModel::GetPtr() const
	{
		if (Diffusion_) return Diffusion_;
		return Reflow_;
	}

	std::vector<std::wstring> UnionSvcModel::Inference(std::wstring& _Paths, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, const InferTools::SlicerSettings& _SlicerSettings) const
	{
		if (Diffusion_) return Diffusion_->Inference(_Paths, _InferParams, _SlicerSettings);
		return Reflow_->Inference(_Paths, _InferParams, _SlicerSettings);
	}

	std::vector<int16_t> UnionSvcModel::InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
	{
		if (Diffusion_) return Diffusion_->InferPCMData(PCMData, srcSr, _InferParams);
		return Reflow_->InferPCMData(PCMData, srcSr, _InferParams);
	}

	std::vector<int16_t> UnionSvcModel::ShallowDiffusionInference(std::vector<float>& _16KAudioHubert, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, std::pair<std::vector<float>, int64_t>& _Mel, const std::vector<float>& _SrcF0, const std::vector<float>& _SrcVolume, const std::vector<std::vector<float>>& _SrcSpeakerMap, size_t& Process, int64_t SrcSize) const
	{
		if (Diffusion_) return Diffusion_->ShallowDiffusionInference(_16KAudioHubert, _InferParams, _Mel, _SrcF0, _SrcVolume, _SrcSpeakerMap, Process, SrcSize);
		return Reflow_->ShallowDiffusionInference(_16KAudioHubert, _InferParams, _Mel, _SrcF0, _SrcVolume, _SrcSpeakerMap, Process, SrcSize);
	}

	std::vector<int16_t> UnionSvcModel::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
	{
		if (Diffusion_) return Diffusion_->SliceInference(_Slice,_InferParams);
		return Reflow_->SliceInference(_Slice, _InferParams);
	}

	std::vector<int16_t> UnionSvcModel::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process) const
	{
		if (Diffusion_) return Diffusion_->SliceInference(_Slice, _InferParams, _Process);
		return Reflow_->SliceInference(_Slice, _InferParams, _Process);
	}

	UnionSvcModel::UnionSvcModel(const MJson& Config, const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback, int ProviderID, int NumThread, int DeviceID)
	{
		if (Config["Type"].GetString() == "DiffSvc")
			Diffusion_ = new MoeVoiceStudioCore::DiffusionSvc(
				Config, Callback,
				MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				DeviceID, NumThread
			);
		else if (Config["Type"].GetString() == "ReflowSvc")
			Reflow_ = new MoeVoiceStudioCore::ReflowSvc(
				Config, Callback,
				MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				DeviceID, NumThread
			);
		else
			LibDLVoiceCodecThrow("Trying To Load VitsSvc Model As Union Model!")
	}

	UnionSvcModel::UnionSvcModel(const MoeVoiceStudioCore::Hparams& Config, const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback, int ProviderID, int NumThread, int DeviceID)
	{
		if ((Config.TensorExtractor.find(L"Diff") != std::wstring::npos) || Config.TensorExtractor.find(L"diff") != std::wstring::npos)
			Diffusion_ = new MoeVoiceStudioCore::DiffusionSvc(
				Config, Callback,
				MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				DeviceID, NumThread
			);
		else
			Reflow_ = new MoeVoiceStudioCore::ReflowSvc(
				Config, Callback,
				MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				DeviceID, NumThread
			);
	}

	UnionSvcModel::~UnionSvcModel()
	{
		delete Diffusion_;
		delete Reflow_;
		Diffusion_ = nullptr;
		Reflow_ = nullptr;
	}

	int64_t UnionSvcModel::GetMaxStep() const
	{
		if (Diffusion_) return Diffusion_->GetMaxStep();
		return Reflow_->GetMaxStep();
	}

	bool UnionSvcModel::OldVersion() const
	{
		if (Diffusion_) return Diffusion_->OldVersion();
		return false;
	}

	const std::wstring& UnionSvcModel::GetDiffSvcVer() const
	{
		if (Diffusion_) return Diffusion_->GetDiffSvcVer();
		return Reflow_->GetReflowSvcVer();
	}

	int64_t UnionSvcModel::GetMelBins() const
	{
		if (Diffusion_) return Diffusion_->GetMelBins();
		return Reflow_->GetMelBins();
	}

	int UnionSvcModel::GetHopSize() const
	{
		if (Diffusion_) return Diffusion_->GetHopSize();
		return Reflow_->GetHopSize();
	}

	int64_t UnionSvcModel::GetHiddenUnitKDims() const
	{
		if (Diffusion_) return Diffusion_->GetHiddenUnitKDims();
		return Reflow_->GetHiddenUnitKDims();
	}

	int64_t UnionSvcModel::GetSpeakerCount() const
	{
		if (Diffusion_) return Diffusion_->GetSpeakerCount();
		return Reflow_->GetSpeakerCount();
	}

	bool UnionSvcModel::CharaMixEnabled() const
	{
		if (Diffusion_) return Diffusion_->CharaMixEnabled();
		return Reflow_->CharaMixEnabled();
	}

	long UnionSvcModel::GetSamplingRate() const
	{
		if (Diffusion_) return Diffusion_->GetSamplingRate();
		return Reflow_->GetSamplingRate();
	}

	void UnionSvcModel::NormMel(std::vector<float>& MelSpec) const
	{
		if (Diffusion_) return Diffusion_->NormMel(MelSpec);
		return Reflow_->NormMel(MelSpec);
	}

	bool UnionSvcModel::IsDiffusion() const
	{
		return Diffusion_;
	}
}

namespace MoeVSModuleManager
{
	bool MoeVoiceStudioCoreInitStat = false;

	MoeVoiceStudioCore::VitsSvc* GlobalVitsSvcModel = nullptr;

	UnionSvcModel* GlobalUnionSvcModel = nullptr;

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
		MoeVSRegisterReflowSampler(L"Eular", ReflowEularSampler);
		MoeVSRegisterReflowSampler(L"Rk4", ReflowRk4Sampler);
		MoeVSRegisterReflowSampler(L"Heun", ReflowHeunSampler);
		MoeVSRegisterReflowSampler(L"Pecece", ReflowPececeSampler);
		MoeVoiceStudioCoreInitStat = true;
	}

	MoeVoiceStudioCore::VitsSvc* GetVitsSvcModel()
	{
		return GlobalVitsSvcModel;
	}

	UnionSvcModel* GetUnionSvcModel()
	{
		return GlobalUnionSvcModel;
	}

	void UnloadVitsSvcModel()
	{
		delete GlobalVitsSvcModel;
		GlobalVitsSvcModel = nullptr;
	}

	void UnloadUnionSvcModel()
	{
		delete GlobalUnionSvcModel;
		GlobalUnionSvcModel = nullptr;
	}

	void LoadVitsSvcModel(const MJson& Config,
	                  const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
	                  int ProviderID, int NumThread, int DeviceID)
	{
		logger.log("[Model Loader] Bonding Progress Event");
		ProgessBar = Callback;
		logger.log("[Model Loader] Progress Event Bonded");
		UnloadVitsSvcModel();
		logger.log("[Model Loader] Empty Cache!");

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

	void LoadUnionSvcModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID)
	{
		logger.log("[Model Loader] Bonding Progress Event");
		ProgessBar = Callback;
		logger.log("[Model Loader] Progress Event Bonded");
		UnloadUnionSvcModel();
		logger.log("[Model Loader] Empty Cache!");

		GlobalUnionSvcModel = new UnionSvcModel(
			Config, Callback, ProviderID, NumThread, DeviceID
		);

		if (VocoderEnabled())
			if (!GetVitsSvcModel() || (!GlobalUnionSvcModel->OldVersion() && GlobalUnionSvcModel->GetDiffSvcVer() == L"DiffusionSvc"))
				SamplingRate = GlobalUnionSvcModel->GetSamplingRate();
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
		const bool DiffusionModelEnabled = GetUnionSvcModel() && VocoderEnabled();
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
		
		if (DiffusionModelEnabled && DiffusionTotalStep > GetUnionSvcModel()->GetMaxStep())
			DiffusionTotalStep = GetUnionSvcModel()->GetMaxStep();
		if (SkipDiffusionStep >= DiffusionTotalStep) 
			SkipDiffusionStep = DiffusionTotalStep / 5;
		if (SkipDiffusionStep == 0) 
			SkipDiffusionStep = 1;
		auto RealDiffSteps = DiffusionTotalStep;
		if (GetUnionSvcModel() && GetUnionSvcModel()->IsDiffusion())
			RealDiffSteps = DiffusionTotalStep % SkipDiffusionStep ? DiffusionTotalStep / SkipDiffusionStep + 1 : DiffusionTotalStep / SkipDiffusionStep;
		if((DiffusionModelEnabled && !GlobalVitsSvcModel) || (DiffusionModelEnabled && ShallowDiffusionEnabled()))
		{
			if (GetUnionSvcModel()->OldVersion())
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
				if (DiffusionModelEnabled && !GetUnionSvcModel()->OldVersion())
				{
					ProgressVal += RealDiffSteps;
					ProgessBar(ProgressVal, TotalSteps);
				}
				logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Jump Empty Segment[" + std::to_wstring(SliceIndex++) + L"]!");
			}
			if (DiffusionModelEnabled && GetUnionSvcModel()->OldVersion())
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

	DlCodecStft::Mel& GetMelOperator()
	{
		return *MelOperator;
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
		const bool DiffusionModelEnabled = GetUnionSvcModel() && VocoderEnabled();
		int64_t SamplingRate_I64 = VitsSamplingRate;
		SamplingRate = VitsSamplingRate;
		if ((DiffusionModelEnabled && !GlobalVitsSvcModel) || (_InferParams.UseShallowDiffusion && DiffusionModelEnabled && ShallowDiffusionEnabled()))
		{
			SamplingRate_I64 = GetUnionSvcModel()->GetSamplingRate();
			SamplingRate = GetUnionSvcModel()->GetSamplingRate();
		}
		if (!_Slice.IsNotMute)
			return { size_t(_Slice.OrgLen * SamplingRate_I64 / 48000), 0i16, std::allocator<int16_t>() };
		std::vector<int16_t> RtnAudio;
		RtnAudio.reserve(size_t(double(_Slice.Audio.size()) * 1.5));

		if (GlobalVitsSvcModel)
		{
			auto BgnTime = clock();
			if(_InferParams.UseShallowDiffusion && DiffusionModelEnabled && !GetUnionSvcModel()->OldVersion() && GetUnionSvcModel()->GetDiffSvcVer() == L"DiffusionSvc")
			{
				auto SpcParams = _InferParams;
				SpcParams._ShallowDiffusionModel = GetUnionSvcModel();
				SpcParams.VocoderSamplingRate = (int)SamplingRate_I64;
				SpcParams.VocoderHopSize = GetUnionSvcModel()->GetHopSize();
				SpcParams.VocoderMelBins = (int)GetUnionSvcModel()->GetMelBins();
				std::swap(SpcParams.SpeakerId, SpcParams.ShallowDiffuisonSpeaker);
				RtnAudio = GlobalVitsSvcModel->SliceInference(_Slice, SpcParams, _Process);
			}
			else if(_InferParams.UseShallowDiffusion && VocoderEnabled() && !GetUnionSvcModel())
			{
				auto SpcParams = _InferParams;
				SpcParams.VocoderSamplingRate = (int)SamplingRate_I64;
				SpcParams.VocoderHopSize = VocoderHopSize;
				SpcParams.VocoderMelBins = VocoderMelBins;
				SpcParams._VocoderModel = MoeVoiceStudioCore::GetCurrentVocoder();
				RtnAudio = GlobalVitsSvcModel->SliceInference(_Slice, SpcParams, _Process);
			}
			else
				RtnAudio = GlobalVitsSvcModel->SliceInference(_Slice, _InferParams, _Process);
			logger.log(("[Inference] Slice Vits Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
		}
		else if (DiffusionModelEnabled)
		{
			const auto BgnTime = clock();
			RtnAudio = GetUnionSvcModel()->SliceInference(_Slice, _InferParams, _Process);
			logger.log(("[Inference] Slice Diffusion Use Time " + std::to_string(clock() - BgnTime) + "ms").c_str());
		}
		else
			LibDLVoiceCodecThrow("You Must Load A Model To Inference!");

		return RtnAudio;
	}

	bool ShallowDiffusionEnabled()
	{
		const bool DiffusionModelEnabled = GetUnionSvcModel() && VocoderEnabled();
		return DiffusionModelEnabled && !GetUnionSvcModel()->OldVersion() && GetUnionSvcModel()->GetDiffSvcVer() == L"DiffusionSvc";
	}
}