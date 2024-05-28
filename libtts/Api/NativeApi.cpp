#include "NativeApi.h"
#include <deque>
#include "../Modules/Modules.hpp"
#include "../Modules/Models/EnvManager.hpp"

struct LibTTSParams
{
	float NoiseScale = 0.3f;                           //噪声修正因子          0-10
	int64_t Seed = 52468;                              //种子
	int64_t SpeakerId = 0;                             //角色ID
	uint64_t SrcSamplingRate = 48000;                  //源采样率
	int64_t SpkCount = 2;                              //模型角色数

	std::vector<float> SpeakerMix;                     //角色混合比例
	float LengthScale = 1.0f;                          //时长修正因子
	float DurationPredictorNoiseScale = 0.8f;          //随机时长预测器噪声修正因子
	float FactorDpSdp = 0.f;                           //随机时长预测器与时长预测器混合比例
	float RestTime = 0.5f;                             //停顿时间，为负数则直接断开音频并创建新音频
	float GateThreshold = 0.66666f;                    //Tacotron2解码器EOS阈值
	int64_t MaxDecodeStep = 2000;                      //Tacotron2最大解码步数
	std::vector<std::wstring> EmotionPrompt;           //情感标记
	std::wstring PlaceHolderSymbol = L"|";             //音素分隔符
	std::string LanguageSymbol = "JP";                 //语言
	std::wstring AdditionalInfo = L"";                 //G2P额外信息
	std::wstring SpeakerName = L"0";                   //角色名
};

struct LibTTSToken
{
	std::wstring Text;                                 //输入文本
	std::vector<std::wstring> Phonemes;                //音素序列
	std::vector<int64_t> Tones;                        //音调序列
	std::vector<int64_t> Durations;                    //时长序列
	std::vector<std::string> Language;                 //语言序列
};

struct LibTTSSeq
{
	std::wstring TextSeq;
	std::vector<LibTTSToken> SlicedTokens;
	std::vector<float> SpeakerMix;                     //角色混合比例
	std::vector<std::wstring> EmotionPrompt;           //情感标记
	std::wstring PlaceHolderSymbol = L"|";             //音素分隔符
	float NoiseScale = 0.3f;                           //噪声修正因子             0-10
	float LengthScale = 1.0f;                          //时长修正因子
	float DurationPredictorNoiseScale = 0.3f;          //随机时长预测器噪声修正因子
	float FactorDpSdp = 0.3f;                          //随机时长预测器与时长预测器混合比例
	float GateThreshold = 0.66666f;                    //Tacotron2解码器EOS阈值
	int64_t MaxDecodeStep = 2000;                      //Tacotron2最大解码步数
	int64_t Seed = 52468;                              //种子
	float RestTime = 0.5f;                             //停顿时间，为负数则直接断开音频并创建新音频
	std::string LanguageSymbol = "ZH";                 //语言标记
	std::wstring SpeakerName = L"0";                   //角色或名称ID
	std::wstring AdditionalInfo = L"";                       //G2P额外信息
};

const wchar_t* LibTTSNullString = L"";
#define LibTTSNullStrCheck(Str) ((Str)?(Str):(LibTTSNullString))
std::deque<std::wstring> ErrorQueue;
size_t MaxErrorCount = 20;
std::mutex ErrorMx;

void RaiseError(const std::wstring& _Msg)
{
	logger.log(_Msg);
	ErrorMx.lock();
	ErrorQueue.emplace_front(_Msg);
	if (ErrorQueue.size() > MaxErrorCount)
		ErrorQueue.pop_back();
}

void LibTTSInit()
{
	MoeVSModuleManager::MoeVoiceStudioCoreInitSetup();
}

INT32 LibTTSSetGlobalEnv(UINT32 ThreadCount, UINT32 DeviceID, UINT32 Provider)
{
	try
	{
		moevsenv::GetGlobalMoeVSEnv().Load(ThreadCount, DeviceID, Provider);
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}
	return 0;
}

BSTR LibTTSGetError(size_t Index)
{
	const auto& Ref = ErrorQueue.at(Index);
	auto Ret = SysAllocString(Ref.c_str());
	ErrorQueue.erase(ErrorQueue.begin() + ptrdiff_t(Index));
	ErrorMx.unlock();
	return Ret;
}

void* LibTTSLoadVocoder(LPWSTR VocoderPath)
{
	if (!VocoderPath)
	{
		RaiseError(L"VocoderPath Could Not Be Null");
		return nullptr;
	}

	const auto& Env = moevsenv::GetGlobalMoeVSEnv();
	try
	{
		return new Ort::Session(*Env.GetEnv(), VocoderPath, *Env.GetSessionOptions());
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return nullptr;
	}
}

INT32 LibTTSUnloadVocoder(void* _Model)
{
	try
	{
		delete (Ort::Session*)_Model;
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

void LibTTSEnableFileLogger(bool _Cond)
{
	MoeSSLogger::GetLogger().enable(_Cond);
}

void LibTTSWriteAudioFile(void* _PCMData, LPWSTR _OutputPath, INT32 _SamplingRate)
{
	InferTools::Wav::WritePCMData(_SamplingRate, 1, *(std::vector<int16_t>*)(_PCMData), _OutputPath);
}