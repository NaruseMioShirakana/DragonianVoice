#pragma once
#include "../../framework.h"
#include "windows.h"
#ifdef __cplusplus
extern "C" {
#endif

	typedef void(*ProgCallback)(size_t, size_t);

	enum LibSvcExecutionProviders { CPU = 0, CUDA = 1, DML = 2 };

	enum LibSvcModelType { Vits, Diffusion, Reflow };

#ifdef _MSC_VER
#pragma pack(push, 4)
#else
#pragma pack(4)
#endif

	struct LibSvcSlicerSettings
	{
		INT32 SamplingRate = 48000;
		double Threshold = 30.;
		double MinLength = 3.;
		INT32 WindowLength = 2048;
		INT32 HopSize = 512;
	};

	struct LibSvcParams
	{
		//通用
		float NoiseScale = 0.3f;                           //噪声修正因子          0-10
		INT64 Seed = 52468;                            //种子
		INT64 SpeakerId = 0;                           //角色ID
		size_t SrcSamplingRate = 48000;                    //源采样率
		INT64 SpkCount = 2;                            //模型角色数

		//SVC
		float IndexRate = 0.f;                             //索引比               0-1
		float ClusterRate = 0.f;                           //聚类比               0-1
		float DDSPNoiseScale = 0.8f;                       //DDSP噪声修正因子      0-10
		float Keys = 0.f;                                  //升降调               -64-64
		size_t MeanWindowLength = 2;                       //均值滤波器窗口大小     1-20
		size_t Pndm = 100;                                 //Diffusion加速倍数    1-200
		size_t Step = 1000;                                //Diffusion总步数      1-1000
		float TBegin = 0.f;
		float TEnd = 1.f;
		LPWSTR Sampler = nullptr;                  //Diffusion采样器
		LPWSTR ReflowSampler = nullptr;           //Reflow采样器
		LPWSTR F0Method = nullptr;                  //F0提取算法
		INT32 UseShallowDiffusion = false;                  //使用浅扩散
		void* _VocoderModel = nullptr;
	};

	struct DiffusionSvcPaths
	{
		LPWSTR Encoder = nullptr;
		LPWSTR Denoise = nullptr;
		LPWSTR Pred = nullptr;
		LPWSTR After = nullptr;
		LPWSTR Alpha = nullptr;
		LPWSTR Naive = nullptr;

		LPWSTR DiffSvc = nullptr;
	};

	struct ReflowSvcPaths
	{
		LPWSTR Encoder = nullptr;
		LPWSTR VelocityFn = nullptr;
		LPWSTR After = nullptr;
	};

	struct VitsSvcPaths
	{
		LPWSTR VitsSvc = nullptr;
	};

	struct LibSvcClusterConfig
	{
		INT64 ClusterCenterSize = 10000;
		LPWSTR Path = nullptr;
		LPWSTR Type = nullptr;
	};

	struct LibSvcHparams
	{
		LPWSTR TensorExtractor = nullptr;
		LPWSTR HubertPath = nullptr;
		DiffusionSvcPaths DiffusionSvc;
		VitsSvcPaths VitsSvc;
		ReflowSvcPaths ReflowSvc;
		LibSvcClusterConfig Cluster;

		INT32 SamplingRate = 22050;

		INT32 HopSize = 320;
		INT64 HiddenUnitKDims = 256;
		INT64 SpeakerCount = 1;
		INT32 EnableCharaMix = false;
		INT32 EnableVolume = false;
		INT32 VaeMode = true;

		INT64 MelBins = 128;
		INT64 Pndms = 100;
		INT64 MaxStep = 1000;
		float SpecMin = -12;
		float SpecMax = 2;
		float Scale = 1000.f;
	};

#ifdef _MSC_VER
#pragma pack(pop)
#else
#pragma pack()
#endif
	typedef void* FloatVector, * DoubleDimsFloatVector, * Int16Vector, * UInt64Vector, * MelType, * SliceType, * SlicesType, * SvcModel, * VocoderModel;
	typedef const void* CFloatVector, * CDoubleDimsFloatVector, * CInt16Vector, * CUInt64Vector, * CMelType, * CSliceType, * CSlicesType;

	LibSvcApi float* LibSvcGetFloatVectorData(FloatVector _Obj);

	LibSvcApi size_t LibSvcGetFloatVectorSize(FloatVector _Obj);

	//DFloatVector

	LibSvcApi FloatVector LibSvcGetDFloatVectorData(DoubleDimsFloatVector _Obj, size_t _Index);

	LibSvcApi size_t LibSvcGetDFloatVectorSize(DoubleDimsFloatVector _Obj);

	//Audio - vector<int16_t>

	LibSvcApi Int16Vector LibSvcAllocateAudio();

	LibSvcApi void LibSvcReleaseAudio(Int16Vector _Obj);

	LibSvcApi void LibSvcSetAudioLength(Int16Vector _Obj, size_t _Size);

	LibSvcApi void LibSvcInsertAudio(Int16Vector _ObjA, Int16Vector _ObjB);

	LibSvcApi short* LibSvcGetAudioData(Int16Vector _Obj);

	LibSvcApi size_t LibSvcGetAudioSize(Int16Vector _Obj);

	//Offset - vector<size_t>

	LibSvcApi UInt64Vector LibSvcAllocateOffset();

	LibSvcApi void LibSvcReleaseOffset(UInt64Vector _Obj);

	LibSvcApi void LibSvcSetOffsetLength(UInt64Vector _Obj, size_t _Size);

	LibSvcApi size_t* LibSvcGetOffsetData(UInt64Vector _Obj);

	LibSvcApi size_t LibSvcGetOffsetSize(UInt64Vector _Obj);

	//Mel - pair<vector<float>, int64_t>

	LibSvcApi MelType LibSvcAllocateMel();

	LibSvcApi void LibSvcReleaseMel(MelType _Obj);

	LibSvcApi FloatVector LibSvcGetMelData(MelType _Obj);

	LibSvcApi INT64 LibSvcGetMelSize(MelType _Obj);

	//Slice - MoeVoiceStudioSvcSlice

	LibSvcApi Int16Vector LibSvcGetAudio(SliceType _Obj);

	LibSvcApi FloatVector LibSvcGetF0(SliceType _Obj);

	LibSvcApi FloatVector LibSvcGetVolume(SliceType _Obj);

	LibSvcApi DoubleDimsFloatVector LibSvcGetSpeaker(SliceType _Obj);

	LibSvcApi INT32 LibSvcGetSrcLength(SliceType _Obj);

	LibSvcApi INT32 LibSvcGetIsNotMute(SliceType _Obj);

	LibSvcApi void LibSvcSetSpeakerMixDataSize(SliceType _Obj, size_t _NSpeaker);

	//Slices - MoeVoiceStudioSvcData

	LibSvcApi SlicesType LibSvcAllocateSliceData();

	LibSvcApi void LibSvcReleaseSliceData(SlicesType _Obj);

	LibSvcApi BSTR LibSvcGetAudioPath(SlicesType _Obj);

	LibSvcApi SliceType LibSvcGetSlice(SlicesType _Obj, size_t _Index);

	LibSvcApi size_t LibSvcGetSliceCount(SlicesType _Obj);

	/******************************************Fun**********************************************/

	LibSvcApi void LibSvcInit();

	LibSvcApi void LibSvcFreeString(BSTR _String);

	LibSvcApi INT32 LibSvcSetGlobalEnv(UINT32 ThreadCount, UINT32 DeviceID, UINT32 Provider);

	LibSvcApi void LibSvcSetMaxErrorCount(size_t Count);

	LibSvcApi BSTR LibSvcGetError(size_t Index);

	LibSvcApi INT32 LibSvcSliceAudio(
		CInt16Vector _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
		const void* _Setting, //Ptr Of LibSvcSlicerSettings
		UInt64Vector _Output //std::vector<size_t> By "LibSvcAllocateOffset()"
	);

	LibSvcApi INT32 LibSvcPreprocess(
		CInt16Vector _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
		CUInt64Vector _SlicePos, //std::vector<size_t> By "LibSvcAllocateOffset()"
		INT32 _SamplingRate,
		INT32 _HopSize,
		double _Threshold,
		const wchar_t* _F0Method, //"Dio" "Harvest" "RMVPE" "FCPE"
		SlicesType _Output // Slices By "LibSvcAllocateSliceData()"
	);

	LibSvcApi INT32 LibSvcStft(
		CInt16Vector _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
		INT32 _SamplingRate,
		INT32 _Hopsize,
		INT32 _MelBins,
		MelType _Output // Mel By "LibSvcAllocateMel()"
	);

	LibSvcApi INT32 LibSvcInferSlice(
		SvcModel _Model, //SingingVoiceConversion Model
		UINT32 _T,
		CSliceType _Slice, // Slices By "LibSvcAllocateSliceData()"
		const void* _InferParams, //Ptr Of LibSvcParams
		size_t* _Process, 
		Int16Vector _Output //std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi INT32 LibSvcShallowDiffusionInference(
		SvcModel _Model, //SingingVoiceConversion Model
		FloatVector _16KAudioHubert, 
		MelType _Mel, //Mel By "LibSvcAllocateMel()"
		CFloatVector _SrcF0, 
		CFloatVector _SrcVolume, 
		CDoubleDimsFloatVector _SrcSpeakerMap, 
		INT64 _SrcSize,
		const void* _InferParams, //Ptr Of LibSvcParams
		size_t* _Process,
		Int16Vector _Output //std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi INT32 LibSvcVocoderEnhance(
		VocoderModel _Model, //Vocoder Model
		MelType _Mel, //Mel By "LibSvcAllocateMel()"
		FloatVector _F0,
		INT32 _VocoderMelBins,
		Int16Vector _Output //std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi SvcModel LibSvcLoadModel(
		UINT32 _T,
		const void* _Config, //Ptr Of LibSvcParams
		ProgCallback _ProgressCallback,
		UINT32 _ExecutionProvider = CPU,
		UINT32 _DeviceID = 0,
		UINT32 _ThreadCount = 0
	);

	LibSvcApi INT32 LibSvcUnloadModel(
		UINT32 _T,
		SvcModel _Model
	);

	LibSvcApi VocoderModel LibSvcLoadVocoder(LPWSTR VocoderPath);

	LibSvcApi INT32 LibSvcUnloadVocoder(VocoderModel _Model);

	LibSvcApi INT32 LibSvcReadAudio(LPWSTR _AudioPath, INT32 _SamplingRate, Int16Vector _Output);

	LibSvcApi void LibSvcEnableFileLogger(bool _Cond);

	LibSvcApi void LibSvcWriteAudioFile(Int16Vector _PCMData, LPWSTR _OutputPath, INT32 _SamplingRate);

#ifdef __cplusplus
}
#endif