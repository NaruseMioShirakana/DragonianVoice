#pragma once
#include <windows.h>
#include <functional>
#include <onnxruntime_cxx_api.h>
#include <regex>
#include <fstream>
#include "../../Helper/Helper.h"
#include "../../StringPreprocess.hpp"
#include "../../PluginApi/pluginApi.hpp"
#include "../../Logger/MoeSSLogger.hpp"
#ifdef max
#undef max
#include "../../Lib/rapidjson/rapidjson.h"
#include "../../Lib/rapidjson/document.h"
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#define INFERCLASSHEADER namespace InferClass{
#define INFERCLASSEND }

#define CUDAMOESS1

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
	Pits
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
};

class BaseModelType
{
public:
	using callback = std::function<void(size_t, size_t)>;
	using callback_params = std::function<InferConfigs()>;
	using int64 = int64_t;
	using MTensor = Ort::Value;
	virtual std::vector<int16_t> Infer(std::wstring&) = 0;
	int InsertMessageToEmptyEditBox(std::wstring& _inputLens) const;
	static std::vector<std::wstring> CutLens(const std::wstring& input);
	static std::map<std::wstring, std::vector<std::wstring>> GetPhonesPairMap(const std::wstring& path);
	static std::map<std::wstring, int64_t> GetPhones(const std::map<std::wstring, std::vector<std::wstring>>& PhonesPair);
	std::vector<InferConfigs> GetParam(std::vector<std::wstring>& input) const;
	void initRegex()
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
	BaseModelType();
protected:
	~BaseModelType();
	Ort::Env* env = nullptr;
	Ort::SessionOptions* session_options = nullptr;
	Ort::MemoryInfo* memory_info = nullptr;
	std::wstring _outputPath = GetCurrentFolder() + L"\\outputs";
	modelType _modelType = modelType::Taco;
	static constexpr long MaxPath = 8000l;
	MoeSSPluginAPI _plugin;
	Mui::MRender* _render = nullptr;
	callback _callback;
	callback_params _get_init_params;

	std::wstring EndString = L">>";
	std::wregex EmoReg, EmoParam, NoiseScaleParam, NoiseScaleWParam, LengthScaleParam, SeedParam, CharaParam, DecodeStepParam, GateParam;
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
INFERCLASSEND