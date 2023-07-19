#include "EnvManager.hpp"
#ifdef _WIN32
#ifdef MOEVSDMLPROVIDER
#include <dml_provider_factory.h>
#endif
#endif
#include <thread>
#include "../Logger/MoeSSLogger.hpp"
#include "../InferTools/F0Extractor/NetF0Predictors/NetF0Predictors.hpp"

MoeVoiceStudioCoreEnvManagerHeader

void MoeVoiceStudioEnv::Destory()
{
	MoeVSF0Extractor::EmptyCache();
	logger.log(L"[Info] Removing Env & Release Memory");
	delete GlobalOrtSessionOptions;
	delete GlobalOrtEnv;
	delete GlobalOrtMemoryInfo;
	GlobalOrtSessionOptions = nullptr;
	GlobalOrtEnv = nullptr;
	GlobalOrtMemoryInfo = nullptr;
	logger.log(L"[Info] Complete!");
}

void MoeVoiceStudioEnv::Load(unsigned ThreadCount, unsigned DeviceID, unsigned Provider)
{
	try
	{
		if (Provider != CurProvider)
			Create(ThreadCount, DeviceID, Provider);
		if (Provider == 0 && ThreadCount != CurThreadCount)
			Create(ThreadCount, DeviceID, Provider);
		if ((Provider == 1 || Provider == 2) && DeviceID != CurDeviceID)
			Create(ThreadCount, DeviceID, Provider);
		CurProvider = Provider;
	}
	catch(std::exception& e)
	{
		Destory();
		CurThreadCount = unsigned(-1);
		CurDeviceID = unsigned(-1);
		CurProvider = unsigned(-1);
		logger.log(to_wide_string(e.what()));
		throw e;
	}
}

void MoeVoiceStudioEnv::Create(unsigned ThreadCount_, unsigned DeviceID_, unsigned ExecutionProvider_)
{
	Destory();
	logger.log(L"[Info] Creating Env");

	switch (ExecutionProvider_)
	{
		case 1:
		{
			const auto AvailableProviders = Ort::GetAvailableProviders();
			bool ret = true;
			for (const auto& it : AvailableProviders)
				if (it.find("CUDA") != std::string::npos)
					ret = false;
			if (ret)
				throw std::exception("CUDA Provider Not Found");
			OrtCUDAProviderOptions cuda_option;
			cuda_option.device_id = int(DeviceID_);
			GlobalOrtSessionOptions = new Ort::SessionOptions;
			GlobalOrtEnv = new Ort::Env(ORT_LOGGING_LEVEL_ERROR, "OnnxModel");
			GlobalOrtSessionOptions->AppendExecutionProvider_CUDA(cuda_option);
			GlobalOrtSessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
			GlobalOrtSessionOptions->SetIntraOpNumThreads(1);
			GlobalOrtMemoryInfo = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
			CurDeviceID = DeviceID_;
			break;
		}
#ifdef MOEVSDMLPROVIDER
		case 2:
		{
			const auto AvailableProviders = Ort::GetAvailableProviders();
			std::string ret;
			for (const auto& it : AvailableProviders)
				if (it.find("Dml") != std::string::npos)
					ret = it;
			if (ret.empty())
				throw std::exception("DML Provider Not Found");
			const OrtApi& ortApi = Ort::GetApi();
			const OrtDmlApi* ortDmlApi = nullptr;
			ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi));

			const Ort::ThreadingOptions threadingOptions;
			GlobalOrtEnv = new Ort::Env(threadingOptions, ORT_LOGGING_LEVEL_ERROR, "");
			GlobalOrtEnv->DisableTelemetryEvents();
			GlobalOrtSessionOptions = new Ort::SessionOptions;
			ortDmlApi->SessionOptionsAppendExecutionProvider_DML(*GlobalOrtSessionOptions, int(DeviceID_));
			GlobalOrtSessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
			GlobalOrtSessionOptions->DisablePerSessionThreads();
			GlobalOrtSessionOptions->SetExecutionMode(ORT_SEQUENTIAL);
			GlobalOrtSessionOptions->DisableMemPattern();
			GlobalOrtMemoryInfo = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU));
			CurDeviceID = DeviceID_;
			break;
		}
#endif
		default:
		{
			if (ThreadCount_ == 0)
				ThreadCount_ = std::thread::hardware_concurrency();
			GlobalOrtSessionOptions = new Ort::SessionOptions;
			GlobalOrtEnv = new Ort::Env(ORT_LOGGING_LEVEL_ERROR, "OnnxModel");
			GlobalOrtSessionOptions->SetIntraOpNumThreads(static_cast<int>(ThreadCount_));
			GlobalOrtSessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
			GlobalOrtMemoryInfo = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
			CurThreadCount = ThreadCount_;
			break;
		}
	}
	logger.log(L"[Info] Env Created");
}

bool MoeVoiceStudioEnv::IsEnabled() const
{
	return GlobalOrtEnv && GlobalOrtMemoryInfo && GlobalOrtSessionOptions;
}

MoeVoiceStudioEnv GlobalMoeVSEnv;

MoeVoiceStudioEnv& GetGlobalMoeVSEnv()
{
	return GlobalMoeVSEnv;
}

MoeVoiceStudioCoreEnvManagerEnd