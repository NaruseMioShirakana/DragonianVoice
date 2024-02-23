#include "../../../header/InferTools/F0Extractor/NetF0Predictors.hpp"
#include <dml_provider_factory.h>
#include "matlabfunctions.h"
#include "../../../header/InferTools/F0Extractor/DioF0Extractor.hpp"
#include "../../../header/Logger/MoeSSLogger.hpp"
#include "../../../header/Models/EnvManager.hpp"
#include "../../../header/InferTools/inferTools.hpp"
#ifdef _WIN32
#include <DXGI.h>
#else
#error
#endif

MoeVoiceStudioF0ExtractorHeader

NetF0Class::NetF0Class()
#ifdef INITF0NETPREDICTOR
{
	try
	{
		wchar_t path[1024];
#ifdef _WIN32
		GetModuleFileName(nullptr, path, 1024);
		std::wstring _curPath = path;
		_curPath = _curPath.substr(0, _curPath.rfind(L'\\'));
		const auto cpath = _curPath + L"/F0Predictor/config.json";
		NetF0PathDir = _curPath + L"/F0Predictor/";
#endif
		const MJson json(to_byte_string(cpath).c_str());
		int DeviceId = 0, Device = 0, NumThread = (int)std::thread::hardware_concurrency();
		if (json["DeviceId"].IsInt())
			DeviceId = json["DeviceId"].GetInt();
		if (json["Device"].IsString())
		{
			const std::string tmp = json["Device"].GetString();
			if (tmp == "CUDA")
				Device = 1;
			else if (tmp == "DML")
				Device = 2;
		}
		if (json["NumThread"].IsInt())
			NumThread = json["NumThread"].GetInt();
		if (Device == 0)
			BuildCPUEnv(NumThread);
		else if (Device == 1)
			BuildCUDAEnv(DeviceId);
		else if (Device == 2)
			BuildDMLEnv(DeviceId);
	}
	catch (std::exception& e)
	{
		logger.log(to_wide_string(e.what()));
		BuildCPUEnv(std::thread::hardware_concurrency());
	}
}
#else
{
	wchar_t path[1024];
#ifdef _WIN32
	GetModuleFileName(nullptr, path, 1024);
	std::wstring _curPath = path;
	_curPath = _curPath.substr(0, _curPath.rfind(L'\\'));
	NetF0PathDir = _curPath + L"/F0Predictor/";
#endif
}
#endif
void NetF0Class::Destory()
{
	delete Model;
	Model = nullptr;
	NetF0Options = nullptr;
	NetF0Env = nullptr;
	Memory = nullptr;
}

void NetF0Class::BuildCPUEnv(unsigned ThreadCount)
{
	Destory();
	NetF0Options = new Ort::SessionOptions;
	NetF0Env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
	NetF0Options->SetIntraOpNumThreads(static_cast<int>(ThreadCount));
	NetF0Options->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
	Memory = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
}

void NetF0Class::BuildCUDAEnv(unsigned Did)
{
	Destory();
	const auto AvailableProviders = Ort::GetAvailableProviders();
	bool ret = true;
	for (const auto& it : AvailableProviders)
		if (it.find("CUDA") != std::string::npos)
			ret = false;
	if (ret)
		LibDLVoiceCodecThrow("CUDA Provider Not Found");
	OrtCUDAProviderOptions cuda_option;
	cuda_option.device_id = int(Did);
	NetF0Options = new Ort::SessionOptions;
	NetF0Env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
	NetF0Options->AppendExecutionProvider_CUDA(cuda_option);
	NetF0Options->SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
	NetF0Options->SetIntraOpNumThreads(1);
	Memory = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
}

void NetF0Class::BuildDMLEnv(unsigned Did)
{
	Destory();
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
	const Ort::ThreadingOptions threadingOptions;
	NetF0Env = new Ort::Env(threadingOptions, ORT_LOGGING_LEVEL_VERBOSE, "");
	NetF0Env->DisableTelemetryEvents();
	NetF0Options = new Ort::SessionOptions;
	ortDmlApi->SessionOptionsAppendExecutionProvider_DML(*NetF0Options, int(Did));
	NetF0Options->SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
	NetF0Options->DisablePerSessionThreads();
	NetF0Options->SetExecutionMode(ORT_SEQUENTIAL);
	NetF0Options->DisableMemPattern();
	Memory = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU));
}

void NetF0Class::LoadModel(const std::wstring& path)
{
	Destory();
	if (!moevsenv::GetGlobalMoeVSEnv().IsEnabled())
		return;
	NetF0Env = moevsenv::GetGlobalMoeVSEnv().GetEnv();
	Memory = moevsenv::GetGlobalMoeVSEnv().GetMemoryInfo();
	NetF0Options = moevsenv::GetGlobalMoeVSEnv().GetSessionOptions();
	try
	{
		Model = new Ort::Session(*NetF0Env, (NetF0PathDir + path).c_str(), *NetF0Options);
	}
	catch (Ort::Exception& e)
	{
		logger.log(to_wide_string(e.what()));
		delete Model;
		Model = nullptr;
	}
}

NetF0Class RMVPECORE;
NetF0Class MELPECORE;

RMVPEF0Extractor::RMVPEF0Extractor(int sampling_rate, int hop_size, int n_f0_bins, double max_f0, double min_f0) :
	BaseF0Extractor(sampling_rate, hop_size, n_f0_bins, max_f0, min_f0)
{
	if (MELPECORE.Model)
		MELPECORE.Destory();
	if (!RMVPECORE.Model)
		RMVPECORE.LoadModel(L"RMVPE.onnx");
}

double average(const double* begin, const double* end)
{
	const auto mp = double(end - begin);
	double sum = 0.;
	while (begin != end)
		sum += *(begin++);
	return sum / mp;
}

std::vector<float> RMVPEF0Extractor::ExtractF0(const std::vector<double>& PCMData, size_t TargetLength)
{
	if (!RMVPECORE.Model)
		return DioF0Extractor((int)fs, (int)hop, (int)f0_bin, f0_max, f0_min).ExtractF0(PCMData, TargetLength);

	const double step = double(fs) / 16000.;
	const auto window_len = (size_t)round(step);
	const auto half_window_len = window_len / 2;
	const auto f0_size = size_t((double)PCMData.size() / step);
	const auto pcm_idx_size = PCMData.size() - 2;
	std::vector pcm(f0_size, 0.f);
	auto idx = double(half_window_len + 1);
	for (size_t i = 0; i < f0_size; ++i)
	{
		const auto index = size_t(round(idx));
		if(index + half_window_len > pcm_idx_size)
			break;
		if (half_window_len == 0)
			pcm[i] = (float)PCMData[index];
		else
			pcm[i] = (float)average(&PCMData[index - half_window_len], &PCMData[index + half_window_len]);
		idx += step;
	}
	std::vector<Ort::Value> Tensors;
	const int64_t pcm_shape[] = { 1, (int64_t)pcm.size() };
	constexpr int64_t one_shape[] = { 1 };
	float threshold[] = { 0.03f };
	Tensors.emplace_back(Ort::Value::CreateTensor(*RMVPECORE.Memory, pcm.data(), pcm.size(), pcm_shape, 2));
	Tensors.emplace_back(Ort::Value::CreateTensor(*RMVPECORE.Memory, threshold, 1, one_shape, 1));

	const auto out = RMVPECORE.Model->Run(Ort::RunOptions{ nullptr },
		InputNames.data(),
		Tensors.data(),
		Tensors.size(),
		OutputNames.data(),
		OutputNames.size());

	const auto osize = out[0].GetTensorTypeAndShapeInfo().GetElementCount();
	const auto of0 = out[0].GetTensorData<float>();
	refined_f0 = std::vector<double>(osize);
	for (size_t i = 0; i < osize; ++i) refined_f0[i] = ((of0[i] > 0.001f) ? (double)out[0].GetTensorData<float>()[i] : NAN);
	InterPf0(TargetLength);
	std::vector<float> finaF0(refined_f0.size());
	for (size_t i = 0; i < refined_f0.size(); ++i) finaF0[i] = isnan(refined_f0[i]) ? 0 : (float)refined_f0[i];
	return finaF0;
}

void RMVPEF0Extractor::InterPf0(size_t TargetLength)
{
	const auto f0Len = refined_f0.size();
	if (abs((int64_t)TargetLength - (int64_t)f0Len) < 3)
	{
		refined_f0.resize(TargetLength, 0.0);
		return;
	}
	for (size_t i = 0; i < f0Len; ++i) if (refined_f0[i] < 0.001) refined_f0[i] = NAN;

	auto xi = arange(0.0, (double)f0Len * (double)TargetLength, (double)f0Len, (double)TargetLength);
	while (xi.size() < TargetLength) xi.emplace_back(*(xi.end() - 1) + ((double)f0Len / (double)TargetLength));
	while (xi.size() > TargetLength) xi.pop_back();

	auto x0 = arange(0, (double)f0Len);
	while (x0.size() < f0Len) x0.emplace_back(*(x0.end() - 1) + 1.);
	while (x0.size() > f0Len) x0.pop_back();

	auto raw_f0 = std::vector<double>(xi.size());
	interp1(x0.data(), refined_f0.data(), static_cast<int>(x0.size()), xi.data(), (int)xi.size(), raw_f0.data());

	for (size_t i = 0; i < xi.size(); i++) if (isnan(raw_f0[i])) raw_f0[i] = 0.0;
	refined_f0 = std::move(raw_f0);
}

MELPEF0Extractor::MELPEF0Extractor(int sampling_rate, int hop_size, int n_f0_bins, double max_f0, double min_f0) :
	BaseF0Extractor(sampling_rate, hop_size, n_f0_bins, max_f0, min_f0)
{
	if (RMVPECORE.Model)
		RMVPECORE.Destory();
	if (!MELPECORE.Model)
		MELPECORE.LoadModel(L"MELPE.onnx");
}

void MELPEF0Extractor::InterPf0(size_t TargetLength)
{
	const auto f0Len = refined_f0.size();
	if (abs((int64_t)TargetLength - (int64_t)f0Len) < 3)
	{
		refined_f0.resize(TargetLength, 0.0);
		return;
	}
	for (size_t i = 0; i < f0Len; ++i) if (refined_f0[i] < 0.001) refined_f0[i] = NAN;

	auto xi = arange(0.0, (double)f0Len * (double)TargetLength, (double)f0Len, (double)TargetLength);
	while (xi.size() < TargetLength) xi.emplace_back(*(xi.end() - 1) + ((double)f0Len / (double)TargetLength));
	while (xi.size() > TargetLength) xi.pop_back();

	auto x0 = arange(0, (double)f0Len);
	while (x0.size() < f0Len) x0.emplace_back(*(x0.end() - 1) + 1.);
	while (x0.size() > f0Len) x0.pop_back();

	auto raw_f0 = std::vector<double>(xi.size());
	interp1(x0.data(), refined_f0.data(), static_cast<int>(x0.size()), xi.data(), (int)xi.size(), raw_f0.data());

	for (size_t i = 0; i < xi.size(); i++) if (isnan(raw_f0[i])) raw_f0[i] = 0.0;
	refined_f0 = std::move(raw_f0);
}

std::vector<float> MELPEF0Extractor::ExtractF0(const std::vector<double>& PCMData, size_t TargetLength)
{
	if (!MELPECORE.Model)
		return DioF0Extractor((int)fs, (int)hop, (int)f0_bin, f0_max, f0_min).ExtractF0(PCMData, TargetLength);

	const double step = double(fs) / 16000.;
	const auto window_len = (size_t)round(step);
	const auto half_window_len = window_len / 2;
	const auto f0_size = size_t((double)PCMData.size() / step);
	const auto pcm_idx_size = PCMData.size() - 2;
	std::vector pcm(f0_size, 0.f);
	auto idx = double(half_window_len + 1);
	for (size_t i = 0; i < f0_size; ++i)
	{
		const auto index = size_t(round(idx));
		if (index + half_window_len > pcm_idx_size)
			break;
		if (half_window_len == 0)
			pcm[i] = (float)PCMData[index];
		else
			pcm[i] = (float)average(&PCMData[index - half_window_len], &PCMData[index + half_window_len]);
		idx += step;
	}
	std::vector<Ort::Value> Tensors;
	const int64_t pcm_shape[] = { 1, (int64_t)pcm.size() };
	Tensors.emplace_back(Ort::Value::CreateTensor(*MELPECORE.Memory, pcm.data(), pcm.size(), pcm_shape, 2));

	const auto out = MELPECORE.Model->Run(Ort::RunOptions{ nullptr },
		InputNames.data(),
		Tensors.data(),
		Tensors.size(),
		OutputNames.data(),
		OutputNames.size());

	const auto osize = out[0].GetTensorTypeAndShapeInfo().GetElementCount();
	const auto of0 = out[0].GetTensorData<float>();
	refined_f0 = std::vector<double>(osize);
	for (size_t i = 0; i < osize; ++i) refined_f0[i] = ((of0[i] > 0.001f) ? (double)out[0].GetTensorData<float>()[i] : NAN);
	InterPf0(TargetLength);
	std::vector<float> finaF0(refined_f0.size());
	for (size_t i = 0; i < refined_f0.size(); ++i) finaF0[i] = isnan(refined_f0[i]) ? 0 : (float)refined_f0[i];
	return finaF0;
}

void EmptyCache()
{
	RMVPECORE.Destory();
	MELPECORE.Destory();
}

MoeVoiceStudioF0ExtractorEnd