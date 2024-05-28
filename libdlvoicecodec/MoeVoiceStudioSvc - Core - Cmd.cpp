#include <iostream>
#include "../libsvc/Api/header/libsvc.h"

//测试用代码
#pragma pack(push, 1)
struct WavHeader {
	char riff[4];        // "RIFF"标志
	DWORD fileSize;      // 文件大小
	char wave[4];        // "WAVE"标志
	char fmt[4];         // "fmt "标志
	DWORD fmtSize;       // 格式数据大小
	WORD audioFormat;    // 音频格式
	WORD numChannels;    // 声道数
	DWORD sampleRate;    // 采样率
	DWORD byteRate;      // 每秒字节数
	WORD blockAlign;     // 数据块对齐
	WORD bitsPerSample;  // 采样位数
	char data[4];        // "data"标志
	DWORD dataSize;      // 音频数据大小
};
#pragma pack(pop)
WavHeader header;
bool ReadWavFile(const wchar_t* filename, std::vector<short>& audioData)
{
	FILE* file = nullptr;
	_wfopen_s(&file, filename, L"rb");
	if (!file) {
		return false;
	}

	fread(&header, sizeof(WavHeader), 1, file);

	if (memcmp(header.riff, "RIFF", 4) != 0 ||
		memcmp(header.wave, "WAVE", 4) != 0 ||
		memcmp(header.fmt, "fmt ", 4) != 0 ||
		memcmp(header.data, "data", 4) != 0) {
		fclose(file);
		return false;
	}

	audioData.resize(header.dataSize / sizeof(short));
	fread(audioData.data(), sizeof(short), audioData.size(), file);

	fclose(file);

	return true;
}

void WriteWavFile(const wchar_t* filename, const std::vector<short>& audioData)
{
	FILE* file = nullptr;
	_wfopen_s(&file, filename, L"wb+");
	if (!file) {
		return;
	}

	fwrite(&header, sizeof(WavHeader), 1, file);

	fwrite(audioData.data(), sizeof(short), audioData.size(), file);

	fclose(file);
}

int main()
{
	libsvc::Init();
	libsvc::Config Config;
	Config.TensorExtractor = L"RVC";
	Config.SamplingRate = 40000;
	Config.HopSize = 320;
	Config.HubertPath = LR"(S:\VSGIT\MoeVoiceStudioSvc - Core - Cmd\x64\Debug\hubert\vec-768-layer-12.onnx)";
	Config.SpeakerCount = 1;
	Config.HiddenUnitKDims = 768;
	Config.VitsSvc.VitsSvc = LR"(S:\VSGIT\MoeVoiceStudioSvc - Core - Cmd\x64\Debug\Models\NaruseMioShirakana\NaruseMioShirakana_RVC.onnx)";
	libsvc::LoadModel(libsvc::ModelType::Vits, Config, L"Shirakana", [](size_t, size_t) {}, MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders::CPU, 0, 8);
	std::vector<int16_t> _Pcm;
	ReadWavFile(L"input.wav", _Pcm);
	InferTools::SlicerSettings _SS;
	_SS.SamplingRate = static_cast<int32_t>(header.sampleRate);
	const auto pos = libsvc::SliceAudio(_Pcm, _SS);
	const auto slices = libsvc::PreprocessSlices(_Pcm, pos, _SS, static_cast<int>(header.sampleRate));
	size_t prop = 0;
	_Pcm.clear();
	for(const auto& i : slices.Slices)
	{
		const auto _Temp = libsvc::InferSlice(libsvc::ModelType::Vits, L"Shirakana", i, {}, prop);
		_Pcm.insert(_Pcm.end(), _Temp.begin(), _Temp.end());
		std::cout << double(++prop) / double(slices.Slices.size()) * 100. << std::endl;
	}
	
	WriteWavFile(L"output.wav", _Pcm);
}