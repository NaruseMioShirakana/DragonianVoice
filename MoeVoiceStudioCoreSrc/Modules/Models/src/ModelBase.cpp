#include "../header/ModelBase.hpp"
#include <thread>
#include <fstream>

InferClass::BaseModelType::BaseModelType()
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
	initRegex();
}

InferClass::BaseModelType::~BaseModelType()
{
	delete session_options;
	delete env;
	delete memory_info;
	env = nullptr;
	session_options = nullptr;
	memory_info = nullptr;
	_plugin.unLoad();
}

int InferClass::BaseModelType::InsertMessageToEmptyEditBox(std::wstring& _inputLens) const
{
	if (_modelType == modelType::Taco || _modelType == modelType::Vits || _modelType == modelType::Pits)
		return -1;
	std::vector<TCHAR> szFileName(MaxPath);
	std::vector<TCHAR> szTitleName(MaxPath);
	OPENFILENAME  ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrFile = szFileName.data();
	ofn.nMaxFile = MaxPath;
	ofn.lpstrFileTitle = szTitleName.data();
	ofn.nMaxFileTitle = MaxPath;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = nullptr;
	if (_modelType == modelType::SoVits || _modelType == modelType::diffSvc)
	{
		constexpr TCHAR szFilter[] = TEXT("音频 (*.wav;*.mp3;*.ogg;*.flac;*.aac)\0*.wav;*.mp3;*.ogg;*.flac;*.aac\0");
		ofn.lpstrFilter = szFilter;
		ofn.lpstrTitle = L"打开音频";
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
	}
	else
	{
		constexpr TCHAR szFilter[] = TEXT("DiffSinger项目文件 (*.json;*.ds)\0*.json;*.ds\0");
		ofn.lpstrFilter = szFilter;
		ofn.lpstrTitle = L"打开项目";
		ofn.lpstrDefExt = TEXT("json");
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
	}
	return 0;
}

std::vector<std::wstring> InferClass::BaseModelType::CutLens(const std::wstring& input)
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

std::map<std::wstring, std::vector<std::wstring>> InferClass::BaseModelType::GetPhonesPairMap(const std::wstring& path)
{
	std::string phoneInfo, phoneInfoAll;
	std::ifstream phonefile(path.c_str());
	if (!phonefile.is_open())
		throw std::exception("phone file not found");
	while (std::getline(phonefile, phoneInfo))
		phoneInfoAll += phoneInfo;
	phonefile.close();
	rapidjson::Document PhoneJson;
	PhoneJson.Parse(phoneInfoAll.c_str());
	if (PhoneJson.HasParseError())
		throw std::exception("json file error");
	std::map<std::wstring, std::vector<std::wstring>> TmpOut;
	for (auto itr = PhoneJson.MemberBegin(); itr != PhoneJson.MemberEnd(); ++itr)
	{
		std::wstring Key = to_wide_string(itr->name.GetString());
		const auto Value = itr->value.GetArray();
		TmpOut[Key] = std::vector<std::wstring>();
		for (const auto& it : Value)
			TmpOut[Key].push_back(to_wide_string(it.GetString()));
	}
	return TmpOut;
}

std::map<std::wstring, int64_t> InferClass::BaseModelType::GetPhones(const std::map<std::wstring, std::vector<std::wstring>>& PhonesPair)
{
	std::map<std::wstring, int64_t> tmpMap;

	for (const auto& it : PhonesPair)
		for (const auto& its : it.second)
			tmpMap[its] = 0;
	tmpMap[L"SP"] = 0;
	tmpMap[L"AP"] = 0;
	int64_t pos = 3;
	for (auto& it : tmpMap)
		it.second = pos++;
	tmpMap[L"PAD"] = 0;
	tmpMap[L"EOS"] = 1;
	tmpMap[L"UNK"] = 2;
	return tmpMap;
}

std::vector<InferClass::InferConfigs> InferClass::BaseModelType::GetParam(std::vector<std::wstring>& input) const
{
	std::vector<InferConfigs> _Lens;
	_Lens.reserve(input.size());
	std::wsmatch match_results;
	for (auto& chari : input)
	{
		InferConfigs Param = _get_init_params();
		if (std::regex_search(chari, match_results, NoiseScaleParam))
			Param.noise_scale = (float)_wtof(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, NoiseScaleWParam))
			Param.noise_scale_w = (float)_wtof(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, LengthScaleParam))
			Param.length_scale = (float)_wtof(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, SeedParam))
			Param.seed = _wtoi(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, CharaParam))
			Param.chara = _wtoi(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, GateParam))
			Param.gateThreshold = (float)_wtof(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, DecodeStepParam))
			Param.maxDecoderSteps = _wtoi(match_results[2].str().c_str());
		if (std::regex_search(chari, match_results, EmoParam))
			Param.emo = match_results[2];
		const auto iter = chari.rfind('}');
		if (iter != std::wstring::npos)
			chari = chari.substr(iter + 2);
		_Lens.push_back(std::move(Param));
	}
	return _Lens;
}
