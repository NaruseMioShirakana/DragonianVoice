#include "../../header/Models/EnvManager.hpp"
#ifdef _WIN32
#ifdef MOEVSDMLPROVIDER
#include <dml_provider_factory.h>
#endif
#endif
#include <thread>
#include "../../header/InferTools/F0Extractor/NetF0Predictors.hpp"
#include "../../header/Modules.hpp"
MoeVoiceStudioCoreEnvManagerHeader

bool SingleOrtApiEnv = false;

bool SingleOrtApiEnvEnabled()
{
	return SingleOrtApiEnv;
}

void UseSingleOrtApiEnv(bool _cond)
{
	SingleOrtApiEnv = _cond;
}

const char* logger_id = "libdlvoicecodec";

void MoeVSOrtLoggingFn(void* param, OrtLoggingLevel severity, const char* category, const char* logid, const char* code_location,
	const char* message)
{
	std::string ort_message = "[";
	ort_message += category;
	ort_message += "; In ";
	ort_message += code_location;
	ort_message += "; By";
	ort_message += logger_id;
	ort_message += "] ";
	ort_message += message;
	logger.log(ort_message.c_str());
}

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

	if (cuda_option_v2)
		Ort::GetApi().ReleaseCUDAProviderOptions(cuda_option_v2);
	cuda_option_v2 = nullptr;
	logger.log(L"[Info] Complete!");
}

void MoeVoiceStudioEnv::Load(unsigned ThreadCount, unsigned DeviceID, unsigned Provider)
{
	if (((Provider != CurProvider) ||
		(Provider == 0 && ThreadCount != CurThreadCount) ||
		((Provider == 1 || Provider == 2) && DeviceID != CurDeviceID)) &&
		(MoeVSModuleManager::GetDiffusionSvcModel() || MoeVSModuleManager::GetVitsSvcModel() || MoeVSModuleManager::VocoderEnabled()))
		LibDLVoiceCodecThrow("A Model Has Been Loaded, You Cannot Change Env When A Model Has Been Loaded");
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
		LibDLVoiceCodecThrow(e.what());
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
				LibDLVoiceCodecThrow("CUDA Provider Not Found");
			GlobalOrtSessionOptions = new Ort::SessionOptions;

#ifdef MoeVSCUDAProviderV1
			OrtCUDAProviderOptions cuda_option;
			cuda_option.device_id = int(DeviceID_);
			cuda_option.do_copy_in_default_stream = false;
			GlobalOrtSessionOptions->AppendExecutionProvider_CUDA(cuda_option);
#else
			const OrtApi& ortApi = Ort::GetApi();
			if (cuda_option_v2)
				ortApi.ReleaseCUDAProviderOptions(cuda_option_v2);
			ortApi.CreateCUDAProviderOptions(&cuda_option_v2);
			const std::vector keys{
				"device_id",
				"gpu_mem_limit",
				"arena_extend_strategy",
				"cudnn_conv_algo_search",
				"do_copy_in_default_stream",
				"cudnn_conv_use_max_workspace",
				"cudnn_conv1d_pad_to_nc1d",
				"enable_cuda_graph",
				"enable_skip_layer_norm_strict_mode"
			};
			const std::vector values{
				std::to_string(DeviceID_).c_str(),
				"2147483648",
				"kNextPowerOfTwo",
				"EXHAUSTIVE",
				"1",
				"1",
				"1",
				"0",
				"0"
			};
			ortApi.UpdateCUDAProviderOptions(cuda_option_v2, keys.data(), values.data(), keys.size());
			GlobalOrtSessionOptions->AppendExecutionProvider_CUDA_V2(*cuda_option_v2);
			//ortApi.ReleaseCUDAProviderOptions(cuda_option_v2);
#endif
			GlobalOrtEnv = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, logger_id, MoeVSOrtLoggingFn, nullptr);
			GlobalOrtSessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
			GlobalOrtSessionOptions->SetIntraOpNumThreads((int)std::thread::hardware_concurrency());
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
				LibDLVoiceCodecThrow("DML Provider Not Found");
			const OrtApi& ortApi = Ort::GetApi();
			const OrtDmlApi* ortDmlApi = nullptr;
			ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi));
			Ort::ThreadingOptions threading_options;
			threading_options.SetGlobalInterOpNumThreads((int)std::thread::hardware_concurrency());
			GlobalOrtEnv = new Ort::Env(threading_options, MoeVSOrtLoggingFn, nullptr, ORT_LOGGING_LEVEL_WARNING, logger_id);
			GlobalOrtEnv->DisableTelemetryEvents();
			GlobalOrtSessionOptions = new Ort::SessionOptions;
			ortDmlApi->SessionOptionsAppendExecutionProvider_DML(*GlobalOrtSessionOptions, int(DeviceID_));
			GlobalOrtSessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
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
			GlobalOrtEnv = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, logger_id, MoeVSOrtLoggingFn, nullptr);
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