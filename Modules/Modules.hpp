#pragma once
#include "Models/header/VitsSvc.hpp"
#include "Models/header/DiffSvc.hpp"
namespace MoeVSModuleManager
{
	inline int64_t SamplingRate = 32000;
	inline int64_t SpeakerCount = 0;

	void MoeVoiceStudioCoreInitSetup();

	MoeVoiceStudioCore::SingingVoiceConversion* GetCurSvcModel();

	void UnloadSvcModel();

	void LoadSvcModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID);
}

namespace MoeVSRename
{
	using MoeVSVitsBasedSvc = MoeVoiceStudioCore::VitsSvc;
	using MoeVSDiffBasedSvc = MoeVoiceStudioCore::DiffusionSvc;
}

