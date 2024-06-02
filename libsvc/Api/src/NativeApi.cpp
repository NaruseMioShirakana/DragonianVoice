#include "../header/NativeApi.h"
#include <deque>
#include <string>
#include "../../Modules/header/Modules.hpp"
#include "../../header/InferTools/Stft/stft.hpp"
#include "../../Modules/header/InferTools/AvCodec/AvCodeResample.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

const wchar_t* LibSvcNullString = L"";

#define LibSvcNullStrCheck(Str) ((Str)?(Str):(LibSvcNullString))

#ifndef _WIN32
BSTR SysAllocString(const wchar_t* _String)
{
	wchar_t* ret = new wchar_t[wcslen(_String)];
	wcscpy(ret, _String);
	return ret;
}

void SysFreeString(BSTR _String)
{
	delete[] _String;
}
#endif

std::deque<std::wstring> ErrorQueue;
size_t MaxErrorCount = 20;

using Config = MoeVoiceStudioCore::Hparams;
using VitsSvc = MoeVoiceStudioCore::VitsSvc;
using UnionSvc = MoeVSModuleManager::UnionSvcModel;
using ReflowSvc = MoeVoiceStudioCore::ReflowSvc;
using ClusterBase = MoeVoiceStudioCluster::MoeVoiceStudioBaseCluster;
using TensorExtractorBase = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor;
using ProgressCallback = MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback;
using ExecutionProvider = MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders;
using Slices = MoeVSProjectSpace::MoeVoiceStudioSvcData;
using SingleSlice = MoeVSProjectSpace::MoeVoiceStudioSvcSlice;
using Params = MoeVSProjectSpace::MoeVSSvcParams;

using AudioContainer = std::vector<int16_t>;
using OffsetContainer = std::vector<size_t>;
using MelContainer = std::pair<std::vector<float>, int64_t>;
using DataContainer = Slices;

std::unordered_map<std::wstring, DlCodecStft::Mel*> MelOperators;

void InitLibSvcHparams(LibSvcHparams* _Input)
{
	_Input->TensorExtractor = nullptr;
	_Input->HubertPath = nullptr;
	_Input->DiffusionSvc = {
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};
	_Input->VitsSvc = {
		nullptr
	};
	_Input->ReflowSvc = {
		nullptr,
		nullptr,
		nullptr
	};
	_Input->Cluster = {
		10000,
		nullptr,
		nullptr
	};

	_Input->SamplingRate = 22050;

	_Input->HopSize = 320;
	_Input->HiddenUnitKDims = 256;
	_Input->SpeakerCount = 1;
	_Input->EnableCharaMix = false;
	_Input->EnableVolume = false;
	_Input->VaeMode = true;

	_Input->MelBins = 128;
	_Input->Pndms = 100;
	_Input->MaxStep = 1000;
	_Input->SpecMin = -12;
	_Input->SpecMax = 2;
	_Input->Scale = 1000.f;
}

void InitLibSvcParams(LibSvcParams* _Input)
{
	//通用
	_Input->NoiseScale = 0.3f;							//噪声修正因子          0-10
	_Input->Seed = 52468;									//种子
	_Input->SpeakerId = 0;								//角色ID
	_Input->SrcSamplingRate = 48000;						//源采样率
	_Input->SpkCount = 2;									//模型角色数

	//SVC
	_Input->IndexRate = 0.f;								//索引比               0-1
	_Input->ClusterRate = 0.f;							//聚类比               0-1
	_Input->DDSPNoiseScale = 0.8f;						//DDSP噪声修正因子      0-10
	_Input->Keys = 0.f;									//升降调               -64-64
	_Input->MeanWindowLength = 2;						//均值滤波器窗口大小     1-20
	_Input->Pndm = 100;									//Diffusion加速倍数    1-200
	_Input->Step = 1000;									//Diffusion总步数      1-1000
	_Input->TBegin = 0.f;
	_Input->TEnd = 1.f;
	_Input->Sampler = nullptr;							//Diffusion采样器
	_Input->ReflowSampler = nullptr;						//Reflow采样器
	_Input->F0Method = nullptr;							//F0提取算法
	_Input->UseShallowDiffusionOrEnhancer = false;                  //使用浅扩散
	_Input->_VocoderModel = nullptr;
	_Input->_ShallowDiffusionModel = nullptr;
	_Input->ShallowDiffusionUseSrcAudio = 1;
	_Input->VocoderHopSize = 512;
	_Input->VocoderMelBins = 128;
	_Input->VocoderSamplingRate = 44100;
	_Input->ShallowDiffuisonSpeaker = 0;
}

void InitLibSvcSlicerSettings(LibSvcSlicerSettings* _Input)
{
	_Input->SamplingRate = 48000;
	_Input->Threshold = 30.;
	_Input->MinLength = 3.;
	_Input->WindowLength = 2048;
	_Input->HopSize = 512;
}

float* LibSvcGetFloatVectorData(void* _Obj)
{
	auto& Obj = *(std::vector<float>*)_Obj;
	return Obj.data();
}

size_t LibSvcGetFloatVectorSize(void* _Obj)
{
	auto& Obj = *(std::vector<float>*)_Obj;
	return Obj.size();
}

void* LibSvcGetDFloatVectorData(void* _Obj, size_t _Index)
{
	auto& Obj = *(std::vector<std::vector<float>>*)_Obj;
	return Obj.data() + _Index;
}

size_t LibSvcGetDFloatVectorSize(void* _Obj)
{
	auto& Obj = *(std::vector<std::vector<float>>*)_Obj;
	return Obj.size();
}

void* LibSvcAllocateAudio()
{
	return new AudioContainer;
}

void* LibSvcAllocateMel()
{
	return new MelContainer;
}

void* LibSvcAllocateOffset()
{
	return new OffsetContainer;
}

void* LibSvcAllocateSliceData()
{
	return new DataContainer;
}

void LibSvcReleaseAudio(void* _Obj)
{
	delete (AudioContainer*)_Obj;
}

void LibSvcReleaseMel(void* _Obj)
{
	delete (MelContainer*)_Obj;
}

void LibSvcReleaseOffset(void* _Obj)
{
	delete (OffsetContainer*)_Obj;
}

void LibSvcSetOffsetLength(void* _Obj, size_t _Size)
{
	auto& Obj = *(OffsetContainer*)_Obj;
	Obj.resize(_Size);
}

void LibSvcReleaseSliceData(void* _Obj)
{
	delete (DataContainer*)_Obj;
}

size_t* LibSvcGetOffsetData(void* _Obj)
{
	auto& Obj = *(OffsetContainer*)_Obj;
	return Obj.data();
}

size_t LibSvcGetOffsetSize(void* _Obj)
{
	auto& Obj = *(OffsetContainer*)_Obj;
	return Obj.size();
}

void LibSvcSetAudioLength(void* _Obj, size_t _Size)
{
	auto& Obj = *(AudioContainer*)_Obj;
	Obj.resize(_Size);
}

void LibSvcInsertAudio(void* _ObjA, void* _ObjB)
{
	auto& ObjA = *(AudioContainer*)_ObjA;
	auto& ObjB = *(AudioContainer*)_ObjB;
	ObjA.insert(ObjA.end(), ObjB.begin(), ObjB.end());
}

int16_t* LibSvcGetAudioData(void* _Obj)
{
	auto& Obj = *(AudioContainer*)_Obj;
	return Obj.data();
}

size_t LibSvcGetAudioSize(void* _Obj)
{
	auto& Obj = *(AudioContainer*)_Obj;
	return Obj.size();
}

void* LibSvcGetMelData(void* _Obj)
{
	auto& Obj = *(MelContainer*)_Obj;
	return &Obj.first;
}

int64_t LibSvcGetMelSize(void* _Obj)
{
	auto& Obj = *(MelContainer*)_Obj;
	return Obj.second;
}

void LibSvcSetMaxErrorCount(size_t Count)
{
	MaxErrorCount = Count;
}

BSTR LibSvcGetAudioPath(void* _Obj)
{
	auto& Obj = *(DataContainer*)_Obj;
	return SysAllocString(Obj.Path.c_str());
}

void* LibSvcGetSlice(void* _Obj, size_t _Index)
{
	auto& Obj = *(DataContainer*)_Obj;
	return Obj.Slices.data() + _Index;
}

size_t LibSvcGetSliceCount(void* _Obj)
{
	auto& Obj = *(DataContainer*)_Obj;
	return Obj.Slices.size();
}

void* LibSvcGetAudio(void* _Obj)
{
	auto& Obj = *(SingleSlice*)_Obj;
	return &Obj.Audio;
}

void* LibSvcGetF0(void* _Obj)
{
	auto& Obj = *(SingleSlice*)_Obj;
	return &Obj.F0;
}

void* LibSvcGetVolume(void* _Obj)
{
	auto& Obj = *(SingleSlice*)_Obj;
	return &Obj.Volume;
}

void* LibSvcGetSpeaker(void* _Obj)
{
	auto& Obj = *(SingleSlice*)_Obj;
	return &Obj.Speaker;
}

INT32 LibSvcGetSrcLength(void* _Obj)
{
	auto& Obj = *(SingleSlice*)_Obj;
	return Obj.OrgLen;
}

INT32 LibSvcGetIsNotMute(void* _Obj)
{
	auto& Obj = *(SingleSlice*)_Obj;
	return Obj.IsNotMute;
}

void LibSvcSetSpeakerMixDataSize(void* _Obj, size_t _NSpeaker)
{
	auto& Obj = *(SingleSlice*)_Obj;
	Obj.Speaker.resize(_NSpeaker, std::vector(Obj.F0.size(), 0.f));
}

void LibSvcInit()
{
	MoeVSModuleManager::MoeVoiceStudioCoreInitSetup();
	moevsenv::UseSingleOrtApiEnv(true);
}

std::mutex ErrorMx;

void LibSvcFreeString(BSTR _String)
{
	SysFreeString(_String);
}

BSTR LibSvcGetError(size_t Index)
{
	const auto& Ref = ErrorQueue.at(Index);
	auto Ret = SysAllocString(Ref.c_str());
	ErrorQueue.erase(ErrorQueue.begin() + ptrdiff_t(Index));
	ErrorMx.unlock();
	return Ret;
}

void RaiseError(const std::wstring& _Msg)
{
	logger.log(_Msg);
	ErrorMx.lock();
	ErrorQueue.emplace_front(_Msg);
	if (ErrorQueue.size() > MaxErrorCount)
		ErrorQueue.pop_back();
}

INT32 LibSvcSetGlobalEnv(UINT32 ThreadCount, UINT32 DeviceID, UINT32 Provider)
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

int32_t LibSvcSliceAudio(
	const void* _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
	const void* _Setting, //LibSvcSlicerSettings
	void* _Output //std::vector<size_t> By "LibSvcAllocateOffset()"
)
{
	if (!_Audio)
	{
		RaiseError(L"Audio Could Not Be Null!");
		return 1;
	}

	if (!_Output)
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	const LibSvcSlicerSettings* _SettingInp = (const LibSvcSlicerSettings*)_Setting;
	auto& Ret = *(std::vector<size_t>*)(_Output);
	InferTools::SlicerSettings SliSetting{
		_SettingInp->SamplingRate,
		_SettingInp->Threshold,
		_SettingInp->MinLength,
		_SettingInp->WindowLength,
		_SettingInp->HopSize
	};
	
	try
	{
		Ret = InferTools::SliceAudio(*(const AudioContainer*)(_Audio), SliSetting);
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}
	return 0;
}

int32_t LibSvcPreprocess(
	const void* _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
	const void* _SlicePos, //std::vector<size_t> By "LibSvcAllocateOffset()"
	int32_t _SamplingRate,
	int32_t _HopSize,
	double _Threshold,
	const wchar_t* _F0Method,
	void* _Output // Slices By "LibSvcAllocateSliceData()"
)
{
	InferTools::SlicerSettings _Setting{
		.Threshold = _Threshold
	};

	if (!_Audio)
	{
		RaiseError(L"Audio Could Not Be Null!");
		return 1;
	}

	if (!_SlicePos)
	{
		RaiseError(L"Slice Pos Could Not Be Null!");
		return 1;
	}

	if(!_Output) 
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	Slices& Ret = *static_cast<Slices*>(_Output);
	try
	{
		Ret = MoeVoiceStudioCore::SingingVoiceConversion::GetAudioSlice(
			*(const AudioContainer*)(_Audio),
			*(const OffsetContainer*)(_SlicePos),
			_Setting
		);
		MoeVoiceStudioCore::SingingVoiceConversion::PreProcessAudio(Ret, _SamplingRate, _HopSize, _F0Method);
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}
	return 0;
}

INT32 LibSvcStft(
	const void* _Audio,
	INT32 _SamplingRate,
	INT32 _Hopsize,
	INT32 _MelBins,
	void* _Output
)
{
	if (!_Audio)
	{
		RaiseError(L"Audio Could Not Be Null!");
		return 1;
	}

	if (!_Output)
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	if (MelOperators.size() > 5)
	{
		delete MelOperators.begin()->second;
		MelOperators.erase(MelOperators.begin());
	}

	try
	{
		const std::wstring _Name = L"S" +
			std::to_wstring(_SamplingRate) +
			L"H" + std::to_wstring(_Hopsize) +
			L"M" + std::to_wstring(_MelBins);
		if (!MelOperators.contains(_Name))
			MelOperators[_Name] = new DlCodecStft::Mel(_Hopsize * 4, _Hopsize, _SamplingRate, _MelBins);
		auto _NormalizedAudio = InferTools::InterpResample(
			*(const AudioContainer*)_Audio,
			_SamplingRate,
			_SamplingRate,
			32768.
		);
		*(MelContainer*)(_Output) = MelOperators.at(_Name)->operator()(_NormalizedAudio);
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

INT32 LibSvcInferSlice(
	void* _Model,
	UINT32 _T,
	const void* _Slice,
	const void* _InferParams,
	size_t* _Process,
	void* _Output
)
{
	if (!_Model)
	{
		RaiseError(L"_Model Could Not Be Null!");
		return 1;
	}

	if (!_Slice)
	{
		RaiseError(L"_Slice Could Not Be Null!");
		return 1;
	}

	if (!_InferParams)
	{
		RaiseError(L"_InferParams Could Not Be Null!");
		return 1;
	}

	if (!_Process)
	{
		RaiseError(L"_Process Could Not Be Null!");
		return 1;
	}

	if (!_Output)
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	const auto& InpParam = *(const LibSvcParams*)(_InferParams);

	if (!InpParam._VocoderModel && _T == 1)
	{
		RaiseError(L"_VocoderModel Could Not Be Null!");
		return 1;
	}

	const Params Param
	{
		InpParam.NoiseScale,
		InpParam.Seed,
		InpParam.SpeakerId,
		InpParam.SrcSamplingRate,
		InpParam.SpkCount,
		InpParam.IndexRate,
		InpParam.ClusterRate,
		InpParam.DDSPNoiseScale,
		InpParam.Keys,
		InpParam.MeanWindowLength,
		InpParam.Pndm,
		InpParam.Step,
		InpParam.TBegin,
		InpParam.TEnd,
		LibSvcNullStrCheck(InpParam.Sampler),
		LibSvcNullStrCheck(InpParam.ReflowSampler),
		LibSvcNullStrCheck(InpParam.F0Method),
		(bool)InpParam.UseShallowDiffusionOrEnhancer,
		InpParam._VocoderModel,
		InpParam._ShallowDiffusionModel,
		(bool)InpParam.ShallowDiffusionUseSrcAudio,
		InpParam.VocoderHopSize,
		InpParam.VocoderMelBins,
		InpParam.VocoderSamplingRate,
		InpParam.ShallowDiffuisonSpeaker
	};

	try
	{
		if (_T == 0)
			*(AudioContainer*)(_Output) = ((VitsSvc*)(_Model))->SliceInference(*(const SingleSlice*)(_Slice), Param, *_Process);
		else if (_T == 1)
			*(AudioContainer*)(_Output) = ((UnionSvc*)(_Model))->SliceInference(*(const SingleSlice*)(_Slice), Param, *_Process);
		else
		{
			RaiseError(L"UnSupported Model Type!");
			return 1;
		}
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

INT32 LibSvcInferPCMData(
	SvcModel _Model,							//SingingVoiceConversion Model
	UINT32 _T,
	CInt16Vector _PCMData,
	const void* _InferParams,					//Ptr Of LibSvcParams
	Int16Vector _Output							//std::vector<int16_t> By "LibSvcAllocateAudio()"
)
{
	if (!_Model)
	{
		RaiseError(L"_Model Could Not Be Null!");
		return 1;
	}

	if (!_PCMData)
	{
		RaiseError(L"_PCMData Could Not Be Null!");
		return 1;
	}

	if (!_InferParams)
	{
		RaiseError(L"_InferParams Could Not Be Null!");
		return 1;
	}

	if (!_Output)
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	const auto& InpParam = *(const LibSvcParams*)(_InferParams);

	if (!InpParam._VocoderModel && _T == 1)
	{
		RaiseError(L"_VocoderModel Could Not Be Null!");
		return 1;
	}

	const Params Param
	{
		InpParam.NoiseScale,
		InpParam.Seed,
		InpParam.SpeakerId,
		InpParam.SrcSamplingRate,
		InpParam.SpkCount,
		InpParam.IndexRate,
		InpParam.ClusterRate,
		InpParam.DDSPNoiseScale,
		InpParam.Keys,
		InpParam.MeanWindowLength,
		InpParam.Pndm,
		InpParam.Step,
		InpParam.TBegin,
		InpParam.TEnd,
		LibSvcNullStrCheck(InpParam.Sampler),
		LibSvcNullStrCheck(InpParam.ReflowSampler),
		LibSvcNullStrCheck(InpParam.F0Method),
		(bool)InpParam.UseShallowDiffusionOrEnhancer,
		InpParam._VocoderModel,
		InpParam._ShallowDiffusionModel,
		(bool)InpParam.ShallowDiffusionUseSrcAudio,
		InpParam.VocoderHopSize,
		InpParam.VocoderMelBins,
		InpParam.VocoderSamplingRate,
		InpParam.ShallowDiffuisonSpeaker
	};

	auto& InputData = *(const AudioContainer*)(_PCMData);

	try
	{
		if (_T == 0)
			*(AudioContainer*)(_Output) = ((VitsSvc*)(_Model))->InferPCMData(InputData, (long)InputData.size(), Param);
		else if (_T == 1)
			*(AudioContainer*)(_Output) = ((UnionSvc*)(_Model))->InferPCMData(InputData, (long)InputData.size(), Param);
		else
		{
			RaiseError(L"UnSupported Model Type!");
			return 1;
		}
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

INT32 LibSvcShallowDiffusionInference(
	void* _Model,
	const void* _16KAudioHubert,
	void* _Mel,
	const void* _SrcF0,
	const void* _SrcVolume,
	const void* _SrcSpeakerMap,
	INT64 _SrcSize,
	const void* _InferParams,
	size_t* _Process,
	void* _Output
)
{
	if (!_Model)
	{
		RaiseError(L"_Model Could Not Be Null!");
		return 1;
	}

	if (!_16KAudioHubert)
	{
		RaiseError(L"_16KAudioHubert Could Not Be Null!");
		return 1;
	}

	if (!_Mel)
	{
		RaiseError(L"_Mel Could Not Be Null!");
		return 1;
	}

	if (!_SrcF0)
	{
		RaiseError(L"_SrcF0 Could Not Be Null!");
		return 1;
	}

	if (!_SrcVolume)
	{
		RaiseError(L"_SrcVolume Could Not Be Null!");
		return 1;
	}

	if (!_SrcSpeakerMap)
	{
		RaiseError(L"_SrcSpeakerMap Could Not Be Null!");
		return 1;
	}

	if (!_InferParams)
	{
		RaiseError(L"_InferParams Could Not Be Null!");
		return 1;
	}

	if (!_Process)
	{
		RaiseError(L"_Process Could Not Be Null!");
		return 1;
	}

	if (!_Output)
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	const auto& InpParam = *(const LibSvcParams*)(_InferParams);

	if(!InpParam._VocoderModel)
	{
		RaiseError(L"_VocoderModel Could Not Be Null!");
		return 1;
	}

	const Params Param
	{
		InpParam.NoiseScale,
		InpParam.Seed,
		InpParam.SpeakerId,
		InpParam.SrcSamplingRate,
		InpParam.SpkCount,
		InpParam.IndexRate,
		InpParam.ClusterRate,
		InpParam.DDSPNoiseScale,
		InpParam.Keys,
		InpParam.MeanWindowLength,
		InpParam.Pndm,
		InpParam.Step,
		InpParam.TBegin,
		InpParam.TEnd,
		LibSvcNullStrCheck(InpParam.Sampler),
		LibSvcNullStrCheck(InpParam.ReflowSampler),
		LibSvcNullStrCheck(InpParam.F0Method),
		(bool)InpParam.UseShallowDiffusionOrEnhancer,
		InpParam._VocoderModel,
		InpParam._ShallowDiffusionModel,
		(bool)InpParam.ShallowDiffusionUseSrcAudio,
		InpParam.VocoderHopSize,
		InpParam.VocoderMelBins,
		InpParam.VocoderSamplingRate,
		InpParam.ShallowDiffuisonSpeaker
	};

	auto _NormalizedAudio = InferTools::InterpResample(
		*(const AudioContainer*)_16KAudioHubert,
		16000,
		16000,
		32768.f
	);

	try
	{
		*(AudioContainer*)(_Output) = ((UnionSvc*)(_Model))->ShallowDiffusionInference(
			_NormalizedAudio,
			Param,
			*(MelContainer*)(_Mel),
			*(const std::vector<float>*)(_SrcF0),
			*(const std::vector<float>*)(_SrcVolume),
			*(const std::vector<std::vector<float>>*)(_SrcSpeakerMap),
			*_Process,
			_SrcSize
		);
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

INT32 LibSvcVocoderEnhance(
	void* _Model,
	void* _Mel,
	const void* _F0,
	INT32 _VocoderMelBins,
	void* _Output
)
{
	if (!_Model)
	{
		RaiseError(L"_Model Could Not Be Null!");
		return 1;
	}

	if (!_F0)
	{
		RaiseError(L"_16KAudioHubert Could Not Be Null!");
		return 1;
	}

	if (!_Mel)
	{
		RaiseError(L"_Mel Could Not Be Null!");
		return 1;
	}

	if (!_Output)
	{
		RaiseError(L"_Output Could Not Be Null!");
		return 1;
	}

	auto Rf0 = *(const std::vector<float>*)(_F0);
	auto& MelTemp = *(MelContainer*)(_Mel);
	if (Rf0.size() != (size_t)MelTemp.second)
		Rf0 = InferTools::InterpFunc(Rf0, (long)Rf0.size(), (long)MelTemp.second);
	try
	{
		*(AudioContainer*)(_Output) = MoeVoiceStudioCore::VocoderInfer(
		   MelTemp.first,
		   Rf0,
		   _VocoderMelBins,
		   MelTemp.second,
		   moevsenv::GetGlobalMoeVSEnv().GetMemoryInfo(),
		   _Model
	   );
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

void* LibSvcLoadModel(
	UINT32 _T,
	const void* _Config,
	ProgCallback _ProgressCallback,
	UINT32 _ExecutionProvider,
	UINT32 _DeviceID,
	UINT32 _ThreadCount
)
{
	if (!_Config)
	{
		RaiseError(L"_Model Could Not Be Null!");
		return nullptr;
	}

	auto& Config = *(const LibSvcHparams*)(_Config);

	//printf("%lld", (long long)(Config.DiffusionSvc.Encoder));

	MoeVoiceStudioCore::Hparams ModelConfig{
		LibSvcNullStrCheck(Config.TensorExtractor),
		LibSvcNullStrCheck(Config.HubertPath),
		{
			LibSvcNullStrCheck(Config.DiffusionSvc.Encoder),
			LibSvcNullStrCheck(Config.DiffusionSvc.Denoise),
			LibSvcNullStrCheck(Config.DiffusionSvc.Pred),
			LibSvcNullStrCheck(Config.DiffusionSvc.After),
			LibSvcNullStrCheck(Config.DiffusionSvc.Alpha),
			LibSvcNullStrCheck(Config.DiffusionSvc.Naive),
			LibSvcNullStrCheck(Config.DiffusionSvc.DiffSvc)
		},
		{
			LibSvcNullStrCheck(Config.VitsSvc.VitsSvc)
		},
		{
			LibSvcNullStrCheck(Config.ReflowSvc.Encoder),
			LibSvcNullStrCheck(Config.ReflowSvc.VelocityFn),
			LibSvcNullStrCheck(Config.ReflowSvc.After)
		},
		{
			Config.Cluster.ClusterCenterSize,
			LibSvcNullStrCheck(Config.Cluster.Path),
			LibSvcNullStrCheck(Config.Cluster.Type)
		},
		Config.SamplingRate,
		Config.HopSize,
		Config.HiddenUnitKDims,
		Config.SpeakerCount,
		(bool)Config.EnableCharaMix,
		(bool)Config.EnableVolume,
		(bool)Config.VaeMode,
		Config.MelBins,
		Config.Pndms,
		Config.MaxStep,
		Config.SpecMin,
		Config.SpecMax,
		Config.Scale
	};
	
	try
	{
		if(_T == 0)
		{
			return new VitsSvc(ModelConfig, _ProgressCallback, static_cast<MoeVoiceStudioCore::MoeVoiceStudioModule::ExecutionProviders>(_ExecutionProvider), _DeviceID, _ThreadCount);
		}
		return new UnionSvc(ModelConfig, _ProgressCallback, int(_ExecutionProvider), int(_DeviceID), int(_ThreadCount));
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return nullptr;
	}
}

INT32 LibSvcUnloadModel(
	UINT32 _T,
	void* _Model
)
{
	try
	{
		if (_T == 0)
			delete (VitsSvc*)_Model;
		else
			delete (UnionSvc*)_Model;
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

void* LibSvcLoadVocoder(LPWSTR VocoderPath)
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

INT32 LibSvcUnloadVocoder(void* _Model)
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

INT32 LibSvcReadAudio(LPWSTR _AudioPath, INT32 _SamplingRate, void* _Output)
{
	try
	{
		*(std::vector<int16_t>*)(_Output) = AudioPreprocess().codec(_AudioPath, _SamplingRate);
	}
	catch (std::exception& e)
	{
		RaiseError(to_wide_string(e.what()));
		return 1;
	}

	return 0;
}

void LibSvcEnableFileLogger(bool _Cond)
{
	MoeSSLogger::GetLogger().enable(_Cond);
}

void LibSvcWriteAudioFile(void* _PCMData, LPWSTR _OutputPath, INT32 _SamplingRate)
{
	InferTools::Wav::WritePCMData(_SamplingRate, 1, *(std::vector<int16_t>*)(_PCMData), _OutputPath);
}