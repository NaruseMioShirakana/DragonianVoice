#pragma once
#include <functional>
#include <onnxruntime_cxx_api.h>
#include <regex>
#include <fstream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include "../../StringPreprocess.hpp"
#include "../../PluginApi/pluginApi.hpp"
#include "../../Logger/MoeSSLogger.hpp"
#include "../../DataStruct/KDTree.hpp"
#include "../../InferTools/Project.hpp"
#ifdef max
#undef max
#include "rapidjson.h"
#include "document.h"
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#include <map>
#define INFERCLASSHEADER namespace InferClass{
#define INFERCLASSEND }

static std::wstring GetCurrentFolder(const std::wstring& defualt = L"")
{
	WCHAR path[MAX_PATH];
#ifdef WIN32
	GetModuleFileName(nullptr, path, MAX_PATH);
	std::wstring _curPath = path;
	_curPath = _curPath.substr(0, _curPath.rfind(L'\\'));
	return _curPath;
#else
	//TODO Other System
	return defualt;
#endif
}

INFERCLASSHEADER
enum class FileType
{
	Audio,
	Image,
	Video,
	ImageVideo,
	DS
};

enum class modelType
{
	Taco,
	Vits,
	SoVits,
	diffSvc,
	diffSinger,
	Pits,
	RVC,
	DDSP
};

struct InferConfigs
{
	long keys = 0;
	long threshold = 30;
	long minLen = 5;
	long frame_len = 4 * 1024;
	long frame_shift = 512;
	int64_t pndm = 100;
	int64_t step = 1000;
	float gateThreshold = 0.666f;
	int64_t maxDecoderSteps = 3000;
	float noise_scale = 0.800f;
	float noise_scale_w = 1.f;
	float length_scale = 1.f;
	int64_t chara = 0;
	int64_t seed = 52468608;
	std::wstring emo;
	bool cp = false;
	size_t filter_window_len = 3;
	bool index = false;
	float index_rate = 1.f;
	float kmeans_rate = 0.f;
	double rt_batch_length = 200.0;
	uint64_t rt_batch_count = 10;
	double rt_cross_fade_length = 0.20;
	long rt_th = 1600;
	std::vector<float> chara_mix;
	bool use_iti = false;
	InferConfigs() = default;
};

class MVSDict
{
public:
	MVSDict() = default;
	~MVSDict() = default;

	[[nodiscard]] bool enabled() const
	{
		return !_Dict.empty();
	}

	void unload()
	{
		_Dict.clear();
	}

	[[nodiscard]] std::vector<std::wstring> DictReplace(const std::vector<std::wstring>& input) const;

	[[nodiscard]] std::vector<std::wstring> DictReplace(const std::wstring& input, const std::wstring& tPlaceholderSymbol) const;

	[[nodiscard]] std::wstring DictReplaceGetStr(const std::wstring& input, const std::wstring& tPlaceholderSymbol, bool usePlaceholderSymbol = true) const;

	void GetDict(const std::wstring& path);

	[[nodiscard]] std::wstring getPlaceholderSymbol() const
	{
		return PlaceholderSymbol;
	}
private:
	std::map<std::wstring, std::vector<std::wstring>> _Dict;
	std::wstring PlaceholderSymbol = L"|";
};

class MVSCleaner
{
public:
	MVSCleaner() = default;

	~MVSCleaner()
	{
		unloadDict();
		unloadG2p();
	}

	void unloadDict()
	{
		_Dict.unload();
	}

	void unloadG2p()
	{
		_G2p.unLoad();
	}

	void loadDict(const std::wstring& _path)
	{
		if (_Dict.enabled())
			unloadDict();
		_Dict.GetDict(_path);
	}

	void loadG2p(const std::wstring& _path)
	{
		if (_G2p.enabled())
			unloadG2p();
		_G2p.Load(_path);
	}

	[[nodiscard]] bool G2pEnabled() const
	{
		return _G2p.enabled();
	}

	[[nodiscard]] bool DictEnabled() const
	{
		return _Dict.enabled();
	}

	[[nodiscard]] std::wstring G2p(const std::wstring& _text) const
	{
		return _G2p.functionAPI(_text);
	}

	[[nodiscard]] auto DictReplace(const std::vector<std::wstring>& input) const 
	{
		return _Dict.DictReplace(input);
	}

	[[nodiscard]] auto DictReplace(const std::wstring& input, const std::wstring& tPlaceholderSymbol) const
	{
		return _Dict.DictReplace(input, tPlaceholderSymbol);
	}

	[[nodiscard]] auto DictReplaceGetStr(const std::wstring& input, const std::wstring& tPlaceholderSymbol, bool usePlaceholderSymbol = true) const
	{
		return _Dict.DictReplaceGetStr(input, tPlaceholderSymbol, usePlaceholderSymbol);
	}

	[[nodiscard]] std::wstring getPlaceholderSymbol() const
	{
		return _Dict.getPlaceholderSymbol();
	}

private:
	MoeSSPluginAPI _G2p;
	MVSDict _Dict;
};

class OnnxModule
{
public:
	enum class Device
	{
		CPU,
		CUDA
	};
	using callback = std::function<void(size_t, size_t)>;
	using callback_params = std::function<InferConfigs()>;
	using int64 = int64_t;
	using MTensor = Ort::Value;

	int InsertMessageToEmptyEditBox(std::wstring& _inputLens) const;
	static std::vector<std::wstring> CutLens(const std::wstring& input);
	void initRegex();
	[[nodiscard]] long GetSamplingRate() const
	{
		return _samplingRate;
	}
	void ChangeDevice(Device _dev);
	[[nodiscard]] std::wstring getEndString() const
	{
		return EndString;
	}
	OnnxModule();
	virtual ~OnnxModule();
	virtual std::vector<int16_t> Inference(std::wstring& _inputLens) const;

	template <typename T = float>
	static void LinearCombination(std::vector<T>& _data, T Value = T(1.0))
	{
		if(_data.empty())
		{
			_data = std::vector<T>(1, Value);
			return;
		}
		T Sum = T(0.0);
		for(const auto& i : _data)
			Sum += i;
		if (Sum < T(0.0001))
		{
			_data = std::vector<T>(_data.size(), T(0.0));
			_data[0] = Value;
			return;
		}
		Sum *= T(Value);
		for (auto& i : _data)
			i /= Sum;
	}
protected:
	Ort::Env* env = nullptr;
	Ort::SessionOptions* session_options = nullptr;
	Ort::MemoryInfo* memory_info = nullptr;

	modelType _modelType = modelType::Taco;
	Device device_ = Device::CPU;

	long _samplingRate = 22050;

	callback _callback;
	callback_params _get_init_params;
	
	std::wstring EndString = L">>";
	std::wregex EmoReg, EmoParam, NoiseScaleParam, NoiseScaleWParam, LengthScaleParam, SeedParam, CharaParam, DecodeStepParam, GateParam;

	static constexpr long MaxPath = 8000l;
	std::wstring _outputPath = GetCurrentFolder() + L"\\outputs";
};

class EmoLoader
{
public:
	static constexpr long startPos = 128;
	EmoLoader() = default;
	EmoLoader(const std::wstring& path)
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
		_wfopen_s(&emofile, path.c_str(), L"r");
		if (!emofile)
			throw std::exception("emoFile not exists");
	}
	~EmoLoader()
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
	}
	void close()
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
	}
	void open(const std::wstring& path)
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
		_wfopen_s(&emofile, path.c_str(), L"rb");
		if (!emofile)
			throw std::exception("emoFile not exists");
	}
	std::vector<float> operator[](long index) const
	{
		if (emofile)
		{
			fseek(emofile, index * 4096 + startPos, SEEK_SET);
			char buffer[4096];
			const auto buf = reinterpret_cast<float*>(buffer);
			const auto bufread = fread_s(buffer, 4096, 1, 4096, emofile);
			if (bufread == 4096)
				return { buf ,buf + 1024 };
			throw std::exception("emo index out of range");
		}
		throw std::exception("emo file not opened");
	}
private:
	FILE* emofile = nullptr;
};

class TTS : public OnnxModule
{
public:
	using DurationCallback = std::function<void(std::vector<float>&)>;

	TTS() = default;

	std::vector<InferConfigs> GetParam(std::vector<std::wstring>& input) const;

	static std::vector<std::vector<bool>> generatePath(float* duration, size_t durationSize, size_t maskSize);

	[[nodiscard]] std::vector<float> GetEmotionVector(std::wstring src) const;

	std::vector<int16_t> Inference(std::wstring& _inputLens) const override;

	[[nodiscard]] virtual std::vector<int16_t> Inference(const MoeVSProject::TTSParams& _input) const;

	[[nodiscard]] std::vector<std::wstring> Inference(const std::vector<MoeVSProject::TTSParams>& _input) const;

	static int64_t find_max_idx(const std::vector<float>& inp)
	{
		int64_t idx = 0;
		for (size_t i = 1; i < inp.size(); ++i)
			if (inp[i] > inp[idx])
				idx = int64_t(i);
		return idx;
	}
	~TTS() override = default;
protected:
	
	DurationCallback _dcb;

	int64_t n_speaker = 1;

	bool CharaMix = false;
	bool add_blank = true;
	bool use_ph = false;
	bool emo = false;

	std::map<std::wstring, int64_t> _Phs;
	std::map<wchar_t, int64_t> _Symbols;
	
	EmoLoader emoLoader;
	std::string emoStringa;
	rapidjson::Document EmoJson;
};

class Kmeans
{
public:
	Kmeans() = default;
	~Kmeans() = default;
	Kmeans(const std::wstring& _path, size_t hidden_size, size_t KmeansLen)
	{
		FILE* file = nullptr;
		_wfopen_s(&file, _path.c_str(), L"rb");
		if (!file)
			throw std::exception("KMeansFileNotExist");
		constexpr long idx = 128;
		fseek(file, idx, SEEK_SET);
		std::vector<float> tmpData(hidden_size);
		const size_t ec = size_t(hidden_size) * sizeof(float);
		std::vector<std::vector<float>> _tmp;
		_tmp.reserve(KmeansLen);
		while (fread(tmpData.data(), 1, ec, file) == ec)
		{
			_tmp.emplace_back(tmpData);
			if (_tmp.size() == KmeansLen)
			{
				_tree.emplace_back(_tmp);
				_tmp.clear();
			}
		}
	}
	std::vector<float> find(const std::vector<float>& point, long chara)
	{
		auto res = _tree[chara].nearest_point(point);
		return res;
	}
private:
	std::vector<KDTree> _tree;
};

class SVC : public OnnxModule
{
public:
	SVC() = default;

	std::vector<MoeVSProject::Params> GetSvcParam(std::wstring& RawPath) const;

	[[nodiscard]] std::vector<int64_t> GetNSFF0(const std::vector<float>&) const;

	static std::vector<float> GetInterpedF0(const std::vector<float>&);

	static std::vector<float> GetUV(const std::vector<float>&);

	static std::vector<int64_t> GetAligments(size_t, size_t);

	template<typename T>
	static std::vector<T> InterpResample(const std::vector<int16_t>& PCMData, long src, long dst)
	{
		if (src != dst)
		{
			const double intstep = double(src) / double(dst);
			const auto xi = arange(0, double(PCMData.size()), intstep);
			auto x0 = arange(0, double(PCMData.size()));
			while (x0.size() < PCMData.size())
				x0.emplace_back(x0[x0.size() - 1] + 1.0);
			while (x0.size() > PCMData.size())
				x0.pop_back();
			std::vector<double> y0(PCMData.size());
			for (size_t i = 0; i < PCMData.size(); ++i)
				y0[i] = double(PCMData[i]) / 32768.0;
			std::vector<double> yi(xi.size());
			interp1(x0.data(), y0.data(), long(x0.size()), xi.data(), long(xi.size()), yi.data());
			std::vector<T> out(xi.size());
			for (size_t i = 0; i < yi.size(); ++i)
				out[i] = T(yi[i]);
			return out;
		}
		std::vector<T> out(PCMData.size());
		for (size_t i = 0; i < PCMData.size(); ++i)
			out[i] = T(PCMData[i]) / (T)(32768.0);
		return out;
	}

	template<typename T>
	static std::vector<T> InterpFunc(const std::vector<T>& Data, long src, long dst)
	{
		if (src != dst)
		{
			const double intstep = double(src) / double(dst);
			auto xi = arange(0, double(Data.size()), intstep);
			while (xi.size() < dst)
				xi.emplace_back(xi[xi.size() - 1] + 1.0);
			while (xi.size() > dst)
				xi.pop_back();
			auto x0 = arange(0, double(Data.size()));
			while (x0.size() < Data.size())
				x0.emplace_back(x0[x0.size() - 1] + 1.0);
			while (x0.size() > Data.size())
				x0.pop_back();
			std::vector<double> y0(Data.size());
			for (size_t i = 0; i < Data.size(); ++i)
				y0[i] = double(Data[i]);
			std::vector<double> yi(xi.size());
			interp1(x0.data(), y0.data(), long(x0.size()), xi.data(), long(xi.size()), yi.data());
			std::vector<T> out(xi.size());
			for (size_t i = 0; i < yi.size(); ++i)
				out[i] = T(yi[i]);
			return out;
		}
		return Data;
	}

	[[nodiscard]] std::vector<float> ExtractVolume(const std::vector<double>& OrgAudio) const;

	[[nodiscard]] std::vector<float> GetInterpedF0log(const std::vector<float>&) const;

	[[nodiscard]] InferConfigs GetInferConfigs() const
	{
		return _get_init_params();
	}

	[[nodiscard]] int GetHopSize() const
	{
		return hop;
	}

	[[nodiscard]] int64_t GetHiddenSize() const
	{
		return Hidden_Size;
	}

	[[nodiscard]] long GetPndm() const
	{
		return pndm;
	}

	[[nodiscard]] long GetMelBins() const
	{
		return melBins;
	}

	[[nodiscard]] int64_t GetNSpeaker() const
	{
		return n_speaker;
	}
	~SVC() override = default;
protected:
	
	Ort::Session* hubert = nullptr;

	int hop = 320;
	int64_t Hidden_Size = 256;
	long pndm = 100;
	long melBins = 256;
	int64_t n_speaker = 1;
	bool CharaMix = false;
	bool VolumeB = false;
	bool V2 = false;
	bool ddsp = false;
	bool SV3 = false;
	bool SV4 = false;
	bool SVV2 = false;

	Kmeans* kmeans_ = nullptr;
	int64_t KMeans_Size = 10000;
	bool KMenas_Stat = false;
	bool Index_Stat = false;

	Ort::AllocatorWithDefaultOptions allocator;

	const int f0_bin = 256;
	const float f0_max = 1100.0;
	const float f0_min = 50.0;
	const float f0_mel_min = 1127.f * log(1.f + f0_min / 700.f);
	const float f0_mel_max = 1127.f * log(1.f + f0_max / 700.f);

	const std::vector<const char*> hubertOutput = { "embed" };
	const std::vector<const char*> hubertInput = { "source" };
};
INFERCLASSEND