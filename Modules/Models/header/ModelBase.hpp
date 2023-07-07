#pragma once
#include <functional>
#include <thread>
#include <onnxruntime_cxx_api.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "MoeVSProject.hpp"

#define MoeVoiceStudioCoreHeader namespace MoeVoiceStudioCore{
#define MoeVoiceStudioCoreEnd }
#define MoeVSNotImplementedError throw std::exception("NotImplementedError")
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

	[[nodiscard]] long GetSamplingRate() const
	{
		return _samplingRate;
	}

protected:
	//采样率
	long _samplingRate = 22050;

	Ort::Env* env = nullptr;
	Ort::SessionOptions* session_options = nullptr;
	Ort::MemoryInfo* memory_info = nullptr;

	ProgressCallback _callback;

	std::vector<std::wstring> __NAME__CLASS__ = { L"MoeVoiceStudioModule" };
public:
	//*******************删除的函数********************//
	MoeVoiceStudioModule& operator=(MoeVoiceStudioModule&&) = delete;
	MoeVoiceStudioModule& operator=(const MoeVoiceStudioModule&) = delete;
	MoeVoiceStudioModule(const MoeVoiceStudioModule&) = delete;
	MoeVoiceStudioModule(MoeVoiceStudioModule&&) = delete;
};

MoeVoiceStudioCoreEnd