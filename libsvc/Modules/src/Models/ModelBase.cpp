#include "../../header/Models/ModelBase.hpp"
#include <commdlg.h>
MoeVoiceStudioCoreHeader

MoeVoiceStudioModule::MoeVoiceStudioModule(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_)
{
	if (moevsenv::SingleOrtApiEnvEnabled())
	{
		OrtApiEnv.Load(ThreadCount_, DeviceID_, (unsigned)ExecutionProvider_);
		_cur_execution_provider = ExecutionProvider_;
		env = OrtApiEnv.GetEnv();
		memory_info = OrtApiEnv.GetMemoryInfo();
		session_options = OrtApiEnv.GetSessionOptions();
	}
	else
	{
		moevsenv::GetGlobalMoeVSEnv().Load(ThreadCount_, DeviceID_, (unsigned)ExecutionProvider_);
		_cur_execution_provider = ExecutionProvider_;
		env = moevsenv::GetGlobalMoeVSEnv().GetEnv();
		memory_info = moevsenv::GetGlobalMoeVSEnv().GetMemoryInfo();
		session_options = moevsenv::GetGlobalMoeVSEnv().GetSessionOptions();
	}
}

MoeVoiceStudioModule::~MoeVoiceStudioModule()
{
	env = nullptr;
	memory_info = nullptr;
	session_options = nullptr;
}

std::vector<std::wstring> MoeVoiceStudioModule::CutLens(const std::wstring& input)
{
	std::vector<std::wstring> _Lens;
	std::wstring _tmpLen;
	for (const auto& chari : input)
	{
		if ((chari == L'\n') || (chari == L'\r')) {
			if (!_tmpLen.empty())
			{
				_Lens.push_back(_tmpLen);
				_tmpLen = L"";
			}
		}
		else {
			_tmpLen += chari;
		}
	}
	return _Lens;
}

std::vector<std::wstring> MoeVoiceStudioModule::GetOpenFileNameMoeVS()
{
	constexpr long MaxPath = 8000;
	std::vector<std::wstring> OFNLIST;
#ifdef WIN32
	std::vector<TCHAR> szFileName(MaxPath);
	std::vector<TCHAR> szTitleName(MaxPath);
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrFile = szFileName.data();
	ofn.nMaxFile = MaxPath;
	ofn.lpstrFileTitle = szTitleName.data();
	ofn.nMaxFileTitle = MaxPath;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = nullptr;
	constexpr TCHAR szFilter[] = TEXT("Audio (*.wav;*.mp3;*.ogg;*.flac;*.aac)\0*.wav;*.mp3;*.ogg;*.flac;*.aac\0");
	ofn.lpstrFilter = szFilter;
	ofn.lpstrTitle = nullptr;
	ofn.lpstrDefExt = TEXT("wav");
	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	if (GetOpenFileName(&ofn))
	{
		auto filePtr = szFileName.data();
		std::wstring preFix = filePtr;
		filePtr += preFix.length() + 1;
		if (!*filePtr)
			OFNLIST.emplace_back(preFix);
		else
		{
			preFix += L'\\';
			while (*filePtr != 0)
			{
				std::wstring thisPath(filePtr);
				OFNLIST.emplace_back(preFix + thisPath);
				filePtr += thisPath.length() + 1;
			}
		}
	}
	if(OFNLIST.empty())
		LibDLVoiceCodecThrow("Please Select Files");
	return OFNLIST;
#else
#endif
}

std::vector<std::wstring> MoeVoiceStudioModule::Inference(std::wstring& _Datas,
	const MoeVSProjectSpace::MoeVSParams& _InferParams,
	const InferTools::SlicerSettings& _SlicerSettings) const
{
	MoeVSNotImplementedError;
}

/*
int OnnxModule::InsertMessageToEmptyEditBox(std::wstring& _inputLens)
{
#ifdef WIN32
	std::vector<TCHAR> szFileName(MaxPath);
	std::vector<TCHAR> szTitleName(MaxPath);
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrFile = szFileName.data();
	ofn.nMaxFile = MaxPath;
	ofn.lpstrFileTitle = szTitleName.data();
	ofn.nMaxFileTitle = MaxPath;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = nullptr;

	constexpr TCHAR szFilter[] = TEXT("Audio (*.wav;*.mp3;*.ogg;*.flac;*.aac)\0*.wav;*.mp3;*.ogg;*.flac;*.aac\0");
	ofn.lpstrFilter = szFilter;
	ofn.lpstrTitle = nullptr;
	ofn.lpstrDefExt = TEXT("wav");
	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	if (GetOpenFileName(&ofn))
	{
		auto filePtr = szFileName.data();
		std::wstring preFix = filePtr;
		filePtr += preFix.length() + 1;
		if (!*filePtr)
			_inputLens = preFix;
		else
		{
			preFix += L'\\';
			while (*filePtr != 0)
			{
				std::wstring thisPath(filePtr);
				_inputLens += preFix + thisPath + L'\n';
				filePtr += thisPath.length() + 1;
			}
		}
	}
	else
		return -2;
	return 0;
#else
#endif
}
 */

/*
void OnnxModule::ChangeDevice(Device _dev)
{
	if (_dev == device_)
		return;
	device_ = _dev;
	delete session_options;
	delete env;
	delete memory_info;
	env = nullptr;
	session_options = nullptr;
	memory_info = nullptr;
	switch (_dev)
	{
		case Device::CUDA:
		{
			const auto AvailableProviders = Ort::GetAvailableProviders();
			bool ret = true;
			for (const auto& it : AvailableProviders)
				if (it.find("CUDA") != std::string::npos)
					ret = false;
			if (ret)
				LibDLVoiceCodecThrow("CUDA Provider Not Found");
			OrtCUDAProviderOptions cuda_option;
			cuda_option.device_id = int(__MoeVSGPUID);
			session_options = new Ort::SessionOptions;
			env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
			session_options->AppendExecutionProvider_CUDA(cuda_option);
			session_options->SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
			session_options->SetIntraOpNumThreads(1);
			memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
			break;
		}
#ifdef MOEVSDMLPROVIDER
		case Device::DML:
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

			const Ort::ThreadingOptions threadingOptions;
			env = new Ort::Env(threadingOptions, ORT_LOGGING_LEVEL_VERBOSE, "");
			env->DisableTelemetryEvents();
			session_options = new Ort::SessionOptions;
			ortDmlApi->SessionOptionsAppendExecutionProvider_DML(*session_options, int(__MoeVSGPUID));
			session_options->SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
			session_options->DisablePerSessionThreads();
			session_options->SetExecutionMode(ORT_SEQUENTIAL);
			session_options->DisableMemPattern();
			memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU));
			break;
		}
#endif
		default:
		{
			session_options = new Ort::SessionOptions;
			env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
			session_options->SetIntraOpNumThreads(static_cast<int>(__MoeVSNumThreads));
			session_options->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
			memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
			break;
		}
	}
}
 */

MoeVoiceStudioCoreEnd