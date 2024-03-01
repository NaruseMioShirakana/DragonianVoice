#include "Modules.hpp"

namespace MoeVSModuleManager
{
	bool MoeVoiceStudioCoreInitStat = false;

	MoeVoiceStudioCore::TextToSpeech* CurTextToSpeechModel = nullptr;

	void MoeVoiceStudioCoreInitSetup()
	{
		if (MoeVoiceStudioCoreInitStat)
			return;
		const auto BasicCleanerDir = GetCurrentFolder() + L"/G2P/BasicCleaner.dll";
		if (_waccess(BasicCleanerDir.c_str(), 0) != -1)
		{
			const auto Cleaner = MoeVSG2P::GetDefCleaner();
			Cleaner->loadG2p(BasicCleanerDir);
			Cleaner->GetCleaner().LoadDict(GetCurrentFolder() + L"/G2P");
			Cleaner->loadDict(GetCurrentFolder() + L"/Dict/BasicDict.json");
		}
		MoeVoiceStudioCoreInitStat = true;
	}

	MoeVoiceStudioCore::TextToSpeech* GetCurTTSModel()
	{
		return CurTextToSpeechModel;
	}

	void UnloadTTSModel()
	{
		delete CurTextToSpeechModel;
		CurTextToSpeechModel = nullptr;
		SamplingRate = 32000;
		SpeakerCount = 0;
	}

	void LoadTTSModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID,
		const MoeVoiceStudioCore::TextToSpeech::DurationCallback& DurationCallback)
	{
		UnloadTTSModel();
		if (Config["Type"].GetString() == "Tacotron" || Config["Type"].GetString() == "Tacotron2")
			throw std::exception("Tacotron Not Support Yet");
		if (Config["Type"].GetString() == "GPT-SoVits")
		{
			MoeVoiceStudioCore::DestoryAllBerts();
			CurTextToSpeechModel = dynamic_cast<MoeVoiceStudioCore::TextToSpeech*>(
				new MoeVoiceStudioCore::GptSoVits(
					Config, Callback, DurationCallback,
					MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
					DeviceID, NumThread
				)
				);
		}
		else
		{
			CurTextToSpeechModel = dynamic_cast<MoeVoiceStudioCore::TextToSpeech*>(
			   new MoeVoiceStudioCore::Vits(
				   Config, Callback, DurationCallback,
				   MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders(ProviderID),
				   DeviceID, NumThread
			   )
			   );
		}
		SamplingRate = CurTextToSpeechModel->GetSamplingRate();
	}

}