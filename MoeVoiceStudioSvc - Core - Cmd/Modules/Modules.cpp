#include "Modules.hpp"
#include "InferTools/F0Extractor/DioF0Extractor/DioF0Extractor.hpp"
#include "InferTools/F0Extractor/F0ExtractorManager.hpp"
#include "InferTools/TensorExtractor/MoeVSCoreTensorExtractor.hpp"
#include "InferTools/Cluster/MoeVSClusterManager.hpp"
#include "InferTools/Cluster/MoeVSKmeansCluster.hpp"
#include "InferTools/F0Extractor/HarvestF0Extractor/HarvestF0Extractor.hpp"
#include "InferTools/Sampler/MoeVSSamplerManager.hpp"
#include "InferTools/Sampler/MoeVSSamplers.hpp"
#include "InferTools/F0Extractor/NetF0Predictors/NetF0Predictors.hpp"

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
	MoeVoiceStudioCore::SingingVoiceConversion* CurSvcModel = nullptr;

	void MoeVoiceStudioCoreInitSetup()
	{
		MoeVSRegisterF0Constructor(L"Dio", DioF0Extractor);
		MoeVSRegisterF0Constructor(L"Harvest", HarvestF0Extractor);
		MoeVSRegisterF0Constructor(L"RMVPE", RMVPEF0Extractor);
		MoeVSRegisterTensorConstructor(L"SoVits2.0", SoVits2TensorExtractor);
		MoeVSRegisterTensorConstructor(L"SoVits3.0", SoVits3TensorExtractor);
		MoeVSRegisterTensorConstructor(L"SoVits4.0", SoVits4TensorExtractor);
		MoeVSRegisterTensorConstructor(L"SoVits4.0-DDSP", SoVits4DDSPTensorExtractor);
		MoeVSRegisterTensorConstructor(L"RVC", RVCTensorExtractor);
		MoeVSRegisterTensorConstructor(L"DiffSvc", DiffSvcTensorExtractor);
		MoeVSRegisterTensorConstructor(L"DiffusionSvc", DiffusionSvcTensorExtractor);
		MoeVSRegisterCluster(L"KMeans", KMeansCluster);
		MoeVSRegisterSampler(L"Pndm", PndmSampler);
		MoeVSRegisterSampler(L"DDim", DDimSampler);
	}

	MoeVoiceStudioCore::SingingVoiceConversion* GetCurSvcModel()
	{
		return CurSvcModel;
	}

	void UnloadSvcModel()
	{
		delete CurSvcModel;
		CurSvcModel = nullptr;
		SamplingRate = 32000;
		SpeakerCount = 0;
	}

	void LoadSvcModel(const MJson& Config,
	                  const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
	                  int ProviderID, int NumThread, int DeviceID)
	{
		UnloadSvcModel();
		if (Config["Type"].GetString() == "DiffSvc")
			CurSvcModel = dynamic_cast<MoeVoiceStudioCore::SingingVoiceConversion*>(
				new MoeVoiceStudioCore::DiffusionSvc(
					Config, Callback,
					MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
					DeviceID, NumThread
				)
				);
		else
		{
			CurSvcModel = dynamic_cast<MoeVoiceStudioCore::SingingVoiceConversion*>(
			   new MoeVoiceStudioCore::VitsSvc(
				   Config, Callback,
				   MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				   DeviceID, NumThread
			   )
			   );
		}
		SamplingRate = CurSvcModel->GetSamplingRate();
		SpeakerCount = CurSvcModel->GetSpeakerCount();
	}

}