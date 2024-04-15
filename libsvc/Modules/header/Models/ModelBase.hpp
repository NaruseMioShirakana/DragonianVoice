/**
 * FileName: ModelBase.hpp
 * Note: MoeVoiceStudioCore Onnx 模型基类
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2022-10-17 Create
*/

#pragma once
#include <functional>
#include <thread>
#include <onnxruntime_cxx_api.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "../../Lib/MJson/MJson.h"
#include "EnvManager.hpp"
#include "MoeVSProject.hpp"
#include "../InferTools/inferTools.hpp"

#define MoeVoiceStudioCoreHeader namespace MoeVoiceStudioCore{
#define MoeVoiceStudioCoreEnd }
#define MoeVSNotImplementedError LibDLVoiceCodecThrow("NotImplementedError")
#define MoeVSClassName(__Moe__VSClassName) __NAME__CLASS__.emplace_back((__Moe__VSClassName))
#define MoeVSMaxPath 1024
static std::wstring GetCurrentFolder(const std::wstring& defualt = L"")
{
	wchar_t path[MoeVSMaxPath];
#ifdef _WIN32
	GetModuleFileName(nullptr, path, MoeVSMaxPath);
	std::wstring _curPath = path;
	_curPath = _curPath.substr(0, _curPath.rfind(L'\\'));
	return _curPath;
#else
	//TODO Other System
#error Other System ToDO
#endif
}

MoeVoiceStudioCoreHeader
struct DiffusionSvcPaths
{
	std::wstring Encoder;
	std::wstring Denoise;
	std::wstring Pred;
	std::wstring After;
	std::wstring Alpha;
	std::wstring Naive;

	std::wstring DiffSvc;
};

struct ReflowSvcPaths
{
	std::wstring Encoder;
	std::wstring VelocityFn;
	std::wstring After;
};

struct VitsSvcPaths
{
	std::wstring VitsSvc;
};

struct ClusterConfig
{
	int64_t ClusterCenterSize = 10000;
	std::wstring Path;
	/**
	 * \brief Type Of Cluster : "KMeans" "Index"
	 */
	std::wstring Type;
};

struct Hparams
{
	/**
	 * \brief Model Version
	 * For VitsSvc : "SoVits2.0" "SoVits3.0" "SoVits4.0" "SoVits4.0-DDSP" "RVC"
	 * For DiffusionSvc : "DiffSvc" "DiffusionSvc"
	 */
	std::wstring TensorExtractor = L"DiffSvc";
	/**
	 * \brief Path Of Hubert Model
	 */
	std::wstring HubertPath;
	/**
	 * \brief Path Of DiffusionSvc Model
	 */
	DiffusionSvcPaths DiffusionSvc;
	/**
	 * \brief Path Of VitsSvc Model
	 */
	VitsSvcPaths VitsSvc;
	/**
	 * \brief Path Of ReflowSvc Model
	 */
	ReflowSvcPaths ReflowSvc;
	/**
	 * \brief Config Of Cluster
	 */
	ClusterConfig Cluster;
	
	long SamplingRate = 22050;

	int HopSize = 320;
	int64_t HiddenUnitKDims = 256;
	int64_t SpeakerCount = 1;
	bool EnableCharaMix = false;
	bool EnableVolume = false;
	bool VaeMode = true;

	int64_t MelBins = 128;
	int64_t Pndms = 100;
	int64_t MaxStep = 1000;
	float SpecMin = -12;
	float SpecMax = 2;
	float Scale = 1000.f;
};

class MoeVoiceStudioModule
{
public:
	//进度条回调
	using ProgressCallback = std::function<void(size_t, size_t)>;

	//Provicer
	enum class ExecutionProviders
	{
		CPU = 0,
		CUDA = 1,
#ifdef MOEVSDMLPROVIDER
		DML = 2
#endif
	};

	static [[nodiscard]] std::vector<std::wstring> GetOpenFileNameMoeVS();

	static std::vector<std::wstring> CutLens(const std::wstring& input);

	/**
	 * \brief 构造Onnx模型基类
	 * \param ExecutionProvider_ ExecutionProvider(可以理解为设备)
	 * \param DeviceID_ 设备ID
	 * \param ThreadCount_ 线程数
	 */
	MoeVoiceStudioModule(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_ = 0);

	virtual ~MoeVoiceStudioModule();

	/**
	 * \brief 输入路径推理
	 * \param _Datas [路径，多个路径使用换行符隔开, 推理文本]
	 * \param _InferParams 推理参数
	 * \param _SlicerSettings 切片机配置
	 * \return 输出路径
	 */
	[[nodiscard]] virtual std::vector<std::wstring> Inference(std::wstring& _Datas,
	                                                          const MoeVSProjectSpace::MoeVSParams& _InferParams,
	                                                          const InferTools::SlicerSettings& _SlicerSettings) const;

	/**
	 * \brief 获取采样率
	 * \return 采样率
	 */
	[[nodiscard]] long GetSamplingRate() const
	{
		return _samplingRate;
	}

	static float Clamp(float in, float min = -1.f, float max = 1.f)
	{
		if (in > max)
			return max;
		if (in < min)
			return min;
		return in;
	}
protected:
	//采样率
	long _samplingRate = 22050;

	Ort::Env* env = nullptr;
	Ort::SessionOptions* session_options = nullptr;
	Ort::MemoryInfo* memory_info = nullptr;
	ExecutionProviders _cur_execution_provider = ExecutionProviders::CPU;
	ProgressCallback _callback;
	moevsenv::MoeVoiceStudioEnv OrtApiEnv;

	std::vector<std::wstring> __NAME__CLASS__ = { L"MoeVoiceStudioModule" };
public:
	//*******************删除的函数********************//
	MoeVoiceStudioModule& operator=(MoeVoiceStudioModule&&) = delete;
	MoeVoiceStudioModule& operator=(const MoeVoiceStudioModule&) = delete;
	MoeVoiceStudioModule(const MoeVoiceStudioModule&) = delete;
	MoeVoiceStudioModule(MoeVoiceStudioModule&&) = delete;
};

MoeVoiceStudioCoreEnd