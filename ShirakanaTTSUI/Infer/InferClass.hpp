#pragma once
#include <string>
#include "../Window/MainWindow.h"
#include "../Helper/Helper.h"
#include "../Infer/inferTools.hpp"
#include <fstream>
#include <codecvt>
#define INFERCLASSHEADER namespace InferClass{
#define INFERCLASSEND }
INFERCLASSHEADER
class BaseModelType
{
public:
	BaseModelType() = delete;
	virtual ~BaseModelType() = 0;
	virtual void InferOnce(const std::wstring&) = 0;
	virtual void loadModel(const std::wstring&, const std::wstring&, const std::wstring&) = 0;
	virtual void clear() = 0;
	void initEnv()
	{
#ifdef CUDAMOESS
		const auto AvailableProviders = Ort::GetAvailableProviders();
		bool ret = true;
		for (const auto& it : AvailableProviders)
			if (it.find("CUDA") != std::string::npos)
				ret = false;
		if (ret)
			throw std::exception("CUDA Provider Not Found");
		OrtCUDAProviderOptions cuda_option;
		cuda_option.device_id = 0;
#endif
#ifdef DMLMOESS
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
#endif
		session_options = new Ort::SessionOptions;
#ifdef CUDAMOESS
		env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
		session_options->AppendExecutionProvider_CUDA(cuda_option);
		session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
		session_options->SetIntraOpNumThreads(1);
		memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
#else
#ifdef DMLMOESS
		ThreadingOptions threadingOptions;
		env = new Ort::Env(threadingOptions.threadingOptions, LoggingFunction, nullptr, ORT_LOGGING_LEVEL_VERBOSE, "");
		env->DisableTelemetryEvents();
		ortDmlApi->SessionOptionsAppendExecutionProvider_DML(*session_options, 0);
		session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
		session_options->DisablePerSessionThreads();
		session_options->SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
		session_options->DisableMemPattern();
		memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU));
#else
		env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
		session_options->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency()));
		session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
		memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
#endif
#endif
	}
	void delEnv()
	{
#ifdef CUDAMOESS
		delete session_options_nsf;
		session_options_nsf = nullptr;
#endif
		delete session_options;
		delete env;
		delete memory_info;
		env = nullptr;
		session_options = nullptr;
		memory_info = nullptr;
	}
	Ort::MemoryInfo* memory_info = nullptr;
protected:
	Ort::Env* env = nullptr;
	Ort::SessionOptions* session_options = nullptr;
#ifdef CUDAMOESS
	Ort::SessionOptions* session_options_nsf = nullptr;
#endif
};

class Tacotron2 : public BaseModelType
{
public:
	Tacotron2() = delete;
	~Tacotron2() override
	{
		delete sessionEncoder;
		delete sessionDecoderIter;
		delete sessionPostNet;
		delete sessionGan;
		sessionEncoder = nullptr;
		sessionDecoderIter = nullptr;
		sessionPostNet = nullptr;
		sessionGan = nullptr;
		delEnv();
	}
	void clear() override
	{
		delete sessionEncoder;
		delete sessionDecoderIter;
		delete sessionPostNet;
		delete sessionGan;
		sessionEncoder = nullptr;
		sessionDecoderIter = nullptr;
		sessionPostNet = nullptr;
		sessionGan = nullptr;
		delEnv();
	}
	void loadModel(const std::wstring& _modelPath, const std::wstring& hifiganPath, const std::wstring& rub) override
	{
		clear();
		initEnv();
		try
		{
			sessionEncoder = new Ort::Session(*env, (_modelPath + L"_encoder.onnx").c_str(), *session_options);
			sessionDecoderIter = new Ort::Session(*env, (_modelPath + L"_decoder_iter.onnx").c_str(), *session_options);
			sessionPostNet = new Ort::Session(*env, (_modelPath + L"_postnet.onnx").c_str(), *session_options);
			sessionGan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + hifiganPath + L".onnx").c_str(), *session_options);
			memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
		}
		catch (Ort::Exception& _exception)
		{
			throw std::exception(_exception.what());
		}
	}
private:
	Ort::Session* sessionEncoder = nullptr;
	Ort::Session* sessionDecoderIter = nullptr;
	Ort::Session* sessionPostNet = nullptr;
	Ort::Session* sessionGan = nullptr;
};
INFERCLASSEND