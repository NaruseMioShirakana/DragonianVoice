#include "../header/ModelBase.hpp"

#include <commdlg.h>
#include <thread>
#include <fstream>

InferClass::BaseModelType::BaseModelType()
{
	logger.log(L"[Info] Creating Env");
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
	logger.log(L"[Info] Env Created");
}

InferClass::BaseModelType::~BaseModelType()
{
	logger.log(L"[Info] Removing Env & Release Memory");
	delete session_options;
	delete env;
	delete memory_info;
	env = nullptr;
	session_options = nullptr;
	memory_info = nullptr;
	logger.log(L"[Info] Complete!");
	if(_plugin.enabled())
	{
		logger.log(L"[Info] Unload Plugins");
		_plugin.unLoad();
		logger.log(L"[Info] Plugins Unloaded");
	}
}

int InferClass::BaseModelType::InsertMessageToEmptyEditBox(std::wstring& _inputLens) const
{
#ifdef WIN32
	if (_modelType == modelType::Taco || _modelType == modelType::Vits || _modelType == modelType::Pits)
		return -1;
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
	if (_modelType == modelType::SoVits || _modelType == modelType::diffSvc || _modelType == modelType::RVC || _modelType == modelType::DDSP)
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
#else
	// TODO Linux
#endif
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

void InferClass::BaseModelType::initRegex()
{
	std::wstring JsonPath = GetCurrentFolder() + L"\\ParamsRegex.json";
	std::string JsonData;
	std::ifstream ParamFiles(JsonPath.c_str());
	if (ParamFiles.is_open())
	{
		std::string JsonLine;
		while (std::getline(ParamFiles, JsonLine))
			JsonData += JsonLine;
		ParamFiles.close();
		rapidjson::Document ConfigJson;
		ConfigJson.Parse(JsonData.c_str());
		try
		{
			std::wstring NoiseScaleStr, NoiseScaleWStr, LengthScaleStr, SeedStr, CharaStr, BeginStr, emoStr, decodeStr, gateStr;
			if (ConfigJson["NoiseScaleParam"].IsNull())
				NoiseScaleStr = L"noise";
			else
				NoiseScaleStr = to_wide_string(ConfigJson["NoiseScaleParam"].GetString());

			if (ConfigJson["NoiseScaleWParam"].IsNull())
				NoiseScaleWStr = L"noisew";
			else
				NoiseScaleWStr = to_wide_string(ConfigJson["NoiseScaleWParam"].GetString());

			if (ConfigJson["LengthScaleParam"].IsNull())
				LengthScaleStr = L"length";
			else
				LengthScaleStr = to_wide_string(ConfigJson["LengthScaleParam"].GetString());

			if (ConfigJson["SeedParam"].IsNull())
				SeedStr = L"seed";
			else
				SeedStr = to_wide_string(ConfigJson["SeedParam"].GetString());

			if (ConfigJson["CharaParam"].IsNull())
				CharaStr = L"chara";
			else
				CharaStr = to_wide_string(ConfigJson["CharaParam"].GetString());

			if (ConfigJson["Begin"].IsNull())
				BeginStr = L"<<";
			else
				BeginStr = to_wide_string(ConfigJson["Begin"].GetString());

			if (ConfigJson["End"].IsNull())
				EndString = L">>";
			else
				EndString = to_wide_string(ConfigJson["End"].GetString());

			if (ConfigJson["EmoParam"].IsNull())
				emoStr = L"emo";
			else
				emoStr = to_wide_string(ConfigJson["EmoParam"].GetString());

			if (ConfigJson["DecodeStep"].IsNull())
				decodeStr = L"dec";
			else
				decodeStr = to_wide_string(ConfigJson["DecodeStep"].GetString());

			if (ConfigJson["Gate"].IsNull())
				gateStr = L"gate";
			else
				gateStr = to_wide_string(ConfigJson["Gate"].GetString());

			NoiseScaleParam = BeginStr + L'(' + NoiseScaleStr + L":)([0-9\\.-]+)" + EndString;
			NoiseScaleWParam = BeginStr + L'(' + NoiseScaleWStr + L":)([0-9\\.-]+)" + EndString;
			LengthScaleParam = BeginStr + L'(' + LengthScaleStr + L":)([0-9\\.-]+)" + EndString;
			SeedParam = BeginStr + L'(' + SeedStr + L":)([0-9-]+)" + EndString;
			CharaParam = BeginStr + L'(' + CharaStr + L":)([0-9]+)" + EndString;
			EmoParam = BeginStr + L'(' + emoStr + L":)([^]+)" + EndString;
			EmoReg = L"[^" + BeginStr + EndString + L",;]+";
			DecodeStepParam = BeginStr + L'(' + decodeStr + L":)([0-9-]+)" + EndString;
			GateParam = BeginStr + L'(' + gateStr + L":)([0-9\\.-]+)" + EndString;
			if (EndString[0] == '\\')
				EndString = EndString.substr(1);
		}
		catch (std::exception& e)
		{
			logger.log(LR"(
				[Warn] Set Regex To Default (
				NoiseScaleParam = L"<<(noise:)([0-9\\.-]+)>>";
				NoiseScaleWParam = L"<<(noisew:)([0-9\\.-]+)>>";
				LengthScaleParam = L"<<(length:)([0-9\\.-]+)>>";
				SeedParam = L"<<(seed:)([0-9-]+)>>";
				CharaParam = L"<<(chara:)([0-9]+)>>";
				EndString = L">>";
				EmoParam = L"<<(emo:)(.)>>";)
				)");
			NoiseScaleParam = L"<<(noise:)([0-9\\.-]+)>>";
			NoiseScaleWParam = L"<<(noisew:)([0-9\\.-]+)>>";
			LengthScaleParam = L"<<(length:)([0-9\\.-]+)>>";
			SeedParam = L"<<(seed:)([0-9-]+)>>";
			CharaParam = L"<<(chara:)([0-9]+)>>";
			EndString = L">>";
			EmoParam = L"<<(emo:)(.)>>";
			DecodeStepParam = L"<<(dec:)([0-9]+)>>";
			GateParam = L"<<(gate:)([0-9\\.-]+)>>";
		}
	}
	else
	{
		NoiseScaleParam = L"<<(noise:)([0-9\\.-]+)>>";
		NoiseScaleWParam = L"<<(noisew:)([0-9\\.-]+)>>";
		LengthScaleParam = L"<<(length:)([0-9\\.-]+)>>";
		SeedParam = L"<<(seed:)([0-9-]+)>>";
		CharaParam = L"<<(chara:)([0-9]+)>>";
		EmoParam = L"<<(emo:)(.)>>";
		DecodeStepParam = L"<<(dec:)([0-9]+)>>";
		GateParam = L"<<(gate:)([0-9\\.-]+)>>";
		EndString = L">>";
	}
}

void InferClass::BaseModelType::ChangeDevice(Device _dev)
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
	case Device::CPU:
	{
		session_options = new Ort::SessionOptions;
		env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
		session_options->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency()));
		session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
		memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
	}
	case Device::CUDA:
	{
		const auto AvailableProviders = Ort::GetAvailableProviders();
		bool ret = true;
		for (const auto& it : AvailableProviders)
			if (it.find("CUDA") != std::string::npos)
				ret = false;
		if (ret)
			throw std::exception("CUDA Provider Not Found");
		OrtCUDAProviderOptions cuda_option;
		cuda_option.device_id = 0;
		session_options = new Ort::SessionOptions;
		env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
		session_options->AppendExecutionProvider_CUDA(cuda_option);
		session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
		session_options->SetIntraOpNumThreads(1);
		memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
	}
	}
}

std::vector<int16_t> InferClass::BaseModelType::Inference(std::wstring& _inputLens) const
{
	throw std::exception("Base");
	return {};
}

std::vector<InferClass::InferConfigs> InferClass::TTS::GetParam(std::vector<std::wstring>& input) const
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
			chari = chari.substr(iter + 1);
		_Lens.push_back(std::move(Param));
	}
	return _Lens;
}

std::vector<float> InferClass::TTS::GetEmotionVector(std::wstring src) const
{
	std::vector<float> dst(1024, 0.0);
	std::wsmatch mat;
	uint64_t mul = 0;
	while (std::regex_search(src, mat, EmoReg))
	{
		long emoId;
		const auto emoStr = to_byte_string(mat.str());
		if (!EmoJson[emoStr.c_str()].Empty())
			emoId = EmoJson[emoStr.c_str()].GetInt();
		else
			emoId = atoi(emoStr.c_str());
		auto emoVec = emoLoader[emoId];
		for (size_t i = 0; i < 1024; ++i)
			dst[i] = dst[i] + (emoVec[i] - dst[i]) / (float)(mul + 1ull);
		src = mat.suffix();
		++mul;
	}
	return dst;
}

std::vector<std::vector<bool>> InferClass::TTS::generatePath(float* duration, size_t durationSize, size_t maskSize)
{
	for (size_t i = 1; i < durationSize; ++i)
		duration[i] = duration[i - 1] + duration[i];
	std::vector<std::vector<bool>> path(durationSize, std::vector<bool>(maskSize, false));
	//const auto path = new float[maskSize * durationSize];
	/*
	for (size_t i = 0; i < maskSize; ++i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] = (j < (size_t)duration[i] ? 1.0f : 0.0f);
	for (size_t i = maskSize - 1; i > 0ull; --i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] -= path[i-1][j];
	 */
	auto dur = (size_t)duration[0];
	for (size_t j = 0; j < dur; ++j)
		path[j][0] = true;
	/*
	for (size_t i = maskSize - 1; i > 0ull; --i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] = (j < (size_t)duration[i] && j >= (size_t)duration[i - 1]);
	std::vector<std::vector<float>> tpath(durationSize, std::vector<float>(maskSize));
	for (size_t i = 0; i < maskSize; ++i)
		for (size_t j = 0; j < durationSize; ++j)
			tpath[j][i] = path[i][j];
	 */
	for (size_t j = maskSize - 1; j > 0ull; --j)
	{
		dur = (size_t)duration[j];
		for (auto i = (size_t)duration[j - 1]; i < dur; ++i)
			path[i][j] = true;
	}
	return path;
}

std::vector<int64_t> InferClass::SVC::GetNSFF0(const std::vector<float>& F0) const
{
	const auto f0Len = F0.size();
	std::vector<int64_t> NSFF0(f0Len);
	for (size_t i = 0; i < f0Len; ++i)
	{
		float f0_mel = 1127.f * log(1.f + F0[i] / 700.f);
		if (f0_mel > 0.f)
			f0_mel = (f0_mel - f0_mel_min) * (float(f0_bin) - 2.f) / (f0_mel_max - f0_mel_min) + 1.f;
		if (f0_mel < 1.f)
			f0_mel = 1.f;
		if (f0_mel > float(f0_bin) - 1.f)
			f0_mel = float(f0_bin) - 1.f;
		NSFF0[i] = (int64_t)round(f0_mel);
	}
	return NSFF0;
}

std::vector<float> InferClass::SVC::GetInterpedF0(const std::vector<float>& F0)
{
	const auto specLen = F0.size();
	std::vector<float> Of0(specLen, 0.0);

	float last_value = 0.0;
	for (size_t i = 0; i < specLen; ++i)
	{
		if (F0[i] <= 0.f)
		{
			size_t j = i + 1;
			for (; j < specLen; ++j)
			{
				if (F0[j] > 0.f)
					break;
			}
			if (j < specLen - 1)
			{
				if (last_value > 0.f)
				{
					const auto step = (F0[j] - F0[i - 1]) / float(j - i);
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[i - 1] + step * float(k - i + 1));
				}
				else
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[j]);
				i = j;
			}
			else
			{
				for (size_t k = i; k < specLen; ++k)
					Of0[k] = float(last_value);
				i = specLen;
			}
		}
		else
		{
			if (i == 0)
			{
				Of0[i] = float(F0[i]);
				continue;
			}
			Of0[i] = float(F0[i - 1]);
			last_value = F0[i];
		}
	}
	return Of0;
}

std::vector<float> InferClass::SVC::GetUV(const std::vector<float>& F0)
{
	const auto specLen = F0.size();
	std::vector<float> ruv(specLen, 1.0);
	for (size_t i = 0; i < specLen; ++i)
	{
		if (F0[i] < 0.001f)
			ruv[i] = 0.f;
	}
	return ruv;
}

std::vector<int64_t> InferClass::SVC::GetAligments(size_t specLen, size_t hubertLen)
{
	std::vector<int64_t> mel2ph(specLen + 1, 0);

	size_t startFrame = 0;
	const double ph_durs = static_cast<double>(specLen) / static_cast<double>(hubertLen);
	for (size_t iph = 0; iph < hubertLen; ++iph)
	{
		const auto endFrame = static_cast<size_t>(round(static_cast<double>(iph) * ph_durs + ph_durs));
		for (auto j = startFrame; j < endFrame + 1; ++j)
			mel2ph[j] = static_cast<long long>(iph) + 1;
		startFrame = endFrame + 1;
	}
	return mel2ph;
}

std::vector<float> InferClass::SVC::ExtractVolume(const std::vector<double>& OrgAudio) const
{
	std::vector<double> Audio;
	Audio.reserve(OrgAudio.size() * 2);
	Audio.insert(Audio.end(), hop, OrgAudio[0]);
	Audio.insert(Audio.end(), OrgAudio.begin(), OrgAudio.end());
	Audio.insert(Audio.end(), hop, OrgAudio[OrgAudio.size() - 1]);
	const size_t n_frames = (OrgAudio.size() / hop) + 1;
	std::vector<float> volume(n_frames);
	for (auto& i : Audio)
		i = pow(i, 2);
	int64_t index = 0;
	for (auto& i : volume)
	{
		i = sqrt((float)getAvg(Audio.begin()._Ptr + index * hop, Audio.begin()._Ptr + (index + 1) * hop));
		++index;
	}
	return volume;
}

std::vector<MoeVSProject::Params> InferClass::SVC::GetSvcParam(std::wstring& RawPath) const
{
	if(RawPath.empty())
	{
		const int ret = InsertMessageToEmptyEditBox(RawPath);
		if (ret == -1)
			throw std::exception("TTS Does Not Support Automatic Completion");
		if (ret == -2)
			throw std::exception("Please Select Files");
	}
	RawPath += L'\n';
	std::vector<std::wstring> _Lens = CutLens(RawPath);
	const auto params = _get_init_params();
	const auto filter_window_len = params.filter_window_len;
	auto tran = static_cast<int>(params.keys);
	auto threshold = static_cast<double>(params.threshold);
	auto minLen = static_cast<unsigned long>(params.minLen);
	auto frame_len = static_cast<unsigned long>(params.frame_len);
	auto frame_shift = static_cast<unsigned long>(params.frame_shift);
	if (frame_shift < 512 || frame_shift > 10240)
		frame_shift = 512;
	if (frame_len < 1024 || frame_len > 20480)
		frame_len = 1024;
	if (frame_shift > frame_len)
	{
		frame_shift = 512;
		frame_len = 4 * 1024;
	}
	if (minLen < 3 || minLen > 30)
		minLen = 3;
	if (threshold < 10.0 || threshold > 2000.0)
		threshold = 30.0;
	std::vector<MoeVSProject::Params> SvcParams;
	for (auto& path : _Lens)
	{
		MoeVSProject::Params ParamsSvc;
		if (path[0] == L'\"')
			path = path.substr(1);
		if (path[path.length() - 1] == L'\"')
			path.pop_back();
		logger.log(L"[Inferring] PreProcessing \"" + path + L"\" Encoder");
		auto RawWav = AudioPreprocess().codec(path, _samplingRate);
		auto HubertWav = AudioPreprocess().codec(path, 16000);
		auto info = cutWav(RawWav, threshold, minLen, static_cast<unsigned short>(frame_len), static_cast<unsigned short>(frame_shift));
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
			if ((info.cutOffset[i] - info.cutOffset[i - 1]) / RawWav.getHeader().bytesPerSec > 60)
				throw std::exception("Reached max slice length, please change slicer param");
		const auto LenFactor = ((double)16000 / (double)_samplingRate);
		std::vector<std::vector<float>> Hidden_Unit, F0, Volume, SpeakerMix;
		std::vector<long> OrgLen;
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
		{
			if (info.cutTag[i - 1])
			{
				auto featureInput = F0PreProcess(_samplingRate, (short)hop);
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto srcPos = info.cutOffset[i - 1] / 2;
				std::vector<double> source(len, 0.0);
				for (unsigned long long j = 0; j < len; ++j)
					source[j] = (double)RawWav[srcPos + j] / 32768.0;

				const auto hubertInLen = (size_t)((double)len * LenFactor);
				std::vector<float> hubertin(hubertInLen, 0.0);
				const auto startPos = (size_t)(((double)info.cutOffset[i - 1] / 2.0) * LenFactor);
				for (size_t j = 0; j < hubertInLen; ++j)
					hubertin[j] = (float)HubertWav[startPos + j] / 32768.0f;

				const int64_t inputShape[3] = { 1i64,1i64,(int64_t)hubertInLen };
				std::vector<Ort::Value> inputTensors;
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, hubertin.data(), hubertInLen, inputShape, 3));
				std::vector<Ort::Value> hubertOut;

				try {
					hubertOut = hubert->Run(Ort::RunOptions{ nullptr },
						hubertInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						hubertOutput.data(),
						hubertOutput.size());
				}
				catch (Ort::Exception& e)
				{
					throw std::exception((std::string("Locate: hubert\n") + e.what()).c_str());
				}
				auto hubertOutData = hubertOut[0].GetTensorMutableData<float>();
				auto hubertOutLen = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
				auto hubertOutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();
				if (hubertOutShape[2] != Hidden_Size)
					throw std::exception("Hidden Size UnMatch");

				std::vector<float> hubOutData(hubertOutData, hubertOutData + hubertOutLen);
				std::vector<float> F0Data;

				F0Data = featureInput.GetOrgF0(source.data(), (int64_t)len, int64_t(len) / hop, tran);

				Volume.emplace_back(ExtractVolume(source));
				const size_t F0DataSize = F0Data.size();
				F0Data = mean_filter(F0Data, filter_window_len);
				F0Data.resize(F0DataSize);

				Hidden_Unit.emplace_back(std::move(hubOutData));
				F0.emplace_back(std::move(F0Data));
				OrgLen.emplace_back(len);
				SpeakerMix.emplace_back();
			}
			else
			{
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				Hidden_Unit.emplace_back();
				F0.emplace_back();
				Volume.emplace_back();
				OrgLen.emplace_back(len);
				SpeakerMix.emplace_back();
			}
		}
		ParamsSvc.OrgLen = std::move(OrgLen);
		ParamsSvc.Hidden_Unit = std::move(Hidden_Unit);
		ParamsSvc.F0 = std::move(F0);
		ParamsSvc.Speaker = std::move(SpeakerMix);
		ParamsSvc.symbolb = std::move(info.cutTag);
		ParamsSvc.paths = path;
		ParamsSvc.Volume = std::move(Volume);
		SvcParams.emplace_back(std::move(ParamsSvc));
	}
	return SvcParams;
}

std::vector<float> InferClass::SVC::GetInterpedF0log(const std::vector<float>& rF0) const
{
	const auto specLen = rF0.size();
	std::vector<float> F0(specLen);
	std::vector<float> Of0(specLen, 0.0);
	for (size_t i = 0; i < specLen; ++i)
	{
		if (!VolumeB)
			F0[i] = log2(rF0[i]);
		else
			F0[i] = rF0[i];
		if (isnan(F0[i]) || isinf(F0[i]))
			F0[i] = 0.f;
	}

	float last_value = 0.0;
	for (size_t i = 0; i < specLen; ++i)
	{
		if (F0[i] <= 0.f)
		{
			size_t j = i + 1;
			for (; j < specLen; ++j)
			{
				if (F0[j] > 0.f)
					break;
			}
			if (j < specLen - 1)
			{
				if (last_value > 0.f)
				{
					const auto step = (F0[j] - F0[i - 1]) / float(j - i);
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[i - 1] + step * float(k - i + 1));
				}
				else
					for (size_t k = i; k < j; ++k)
						Of0[k] = float(F0[j]);
				i = j;
			}
			else
			{
				for (size_t k = i; k < specLen; ++k)
					Of0[k] = float(last_value);
				i = specLen;
			}
		}
		else
		{
			Of0[i] = float(F0[i - 1]);
			last_value = F0[i];
		}
	}
	return Of0;
}