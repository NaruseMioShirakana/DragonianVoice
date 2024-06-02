#pragma once
#ifdef LIBSVC_EXPORTS
#define LibSvcApi __declspec(dllexport)
#else
#ifndef LibSvcStaticLib
#define LibSvcApi __declspec(dllimport)
#else
#define LibSvcApi
#endif
#endif

#ifdef __GNUC__
#define LibSvcDeprecated __attribute__((deprecated))
#else
#ifdef _MSC_VER
#define LibSvcDeprecated __declspec(deprecated)
#endif
#endif
#ifdef _WIN32
#include "windows.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
	typedef signed char         INT8, * PINT8;
	typedef signed short        INT16, * PINT16;
	typedef signed int          INT32, * PINT32;
	typedef signed long long      INT64, * PINT64;
	typedef unsigned char       UINT8, * PUINT8;
	typedef unsigned short      UINT16, * PUINT16;
	typedef unsigned int        UINT32, * PUINT32;
	typedef unsigned long long    UINT64, * PUINT64;
	typedef wchar_t* NWPSTR, * LPWSTR, * PWSTR, * BSTR;
#endif

	typedef void(*ProgCallback)(size_t, size_t);

	enum LibSvcExecutionProviders
	{
		CPU = 0,
		CUDA = 1,
		DML = 2
	};

	enum LibSvcModelType
	{
		Vits,
		Diffusion,
		Reflow
	};

#ifdef _MSC_VER
#pragma pack(push, 4)
#else
#pragma pack(4)
#endif

	struct LibSvcSlicerSettings
	{
		INT32 SamplingRate;
		double Threshold;
		double MinLength;
		INT32 WindowLength;
		INT32 HopSize;
	};

	struct LibSvcParams
	{
		//通用
		float NoiseScale;								//噪声修正因子				[   0 ~ 10   ]
		INT64 Seed;										//种子						[   INT64    ]
		INT64 SpeakerId;								//默认角色ID					[   0 ~ NS   ]
		size_t SrcSamplingRate;							//源采样率					[     SR     ]
		INT64 SpkCount;									//模型角色数					[	  NS     ]

		//SVC
		float IndexRate;								//索引比						[   0 ~ 1    ]
		float ClusterRate;								//聚类比						[   0 ~ 1    ]
		float DDSPNoiseScale;							//DDSP噪声修正因子			[   0 ~ 10   ]
		float Keys;										//升降调						[ -64 ~ 64   ]
		size_t MeanWindowLength;						//均值滤波器窗口大小			[   1 ~ 20   ]
		size_t Pndm;									//Diffusion加速倍数			[   1 ~ 200  ]
		size_t Step;									//Diffusion总步数			[   1 ~ 1000 ]
		float TBegin;									//Reflow起始点
		float TEnd;										//Reflow终止点
		LPWSTR Sampler;									//Diffusion采样器			["Pndm" "DDim"]
		LPWSTR ReflowSampler;							//Reflow采样器				["Eular" "Rk4" "Heun" "Pecece"]
		LPWSTR F0Method;								//F0提取算法					["Dio" "Harvest" "RMVPE" "FCPE"]
		INT32 UseShallowDiffusionOrEnhancer;			//是否使用浅扩散/声码器增强		[0(false)/1(true)]
		void* _VocoderModel;							//声码器模型					Diffusion模型必须设定该项目
		void* _ShallowDiffusionModel;                   //扩散模型					浅扩散必需设置为扩散模型地址
		INT32 ShallowDiffusionUseSrcAudio;              //浅扩散模型是否使用原始音频		[0(false)/1(true)]
		INT32 VocoderHopSize;							//声码器HopSize				[    Hop     ]
		INT32 VocoderMelBins;							//声码器MelBins				[    Bins    ]
		INT32 VocoderSamplingRate;						//声码器采样率				[     SR     ]
		INT64 ShallowDiffuisonSpeaker;					//浅扩散中Vits模型输入的角色ID	[   0 ~ NS   ]
	};

	struct DiffusionSvcPaths
	{
		LPWSTR Encoder;
		LPWSTR Denoise;
		LPWSTR Pred;
		LPWSTR After;
		LPWSTR Alpha;
		LPWSTR Naive;

		LPWSTR DiffSvc;
	};

	struct ReflowSvcPaths
	{
		LPWSTR Encoder;
		LPWSTR VelocityFn;
		LPWSTR After;
	};

	struct VitsSvcPaths
	{
		LPWSTR VitsSvc;
	};

	struct LibSvcClusterConfig
	{
		INT64 ClusterCenterSize;
		LPWSTR Path;
		LPWSTR Type; //"KMeans" "Index"
	};

	struct LibSvcHparams
	{
		LPWSTR TensorExtractor;
		LPWSTR HubertPath;
		DiffusionSvcPaths DiffusionSvc;
		VitsSvcPaths VitsSvc;
		ReflowSvcPaths ReflowSvc;
		LibSvcClusterConfig Cluster;

		INT32 SamplingRate;

		INT32 HopSize;
		INT64 HiddenUnitKDims;
		INT64 SpeakerCount;
		INT32 EnableCharaMix;
		INT32 EnableVolume;
		INT32 VaeMode;

		INT64 MelBins;
		INT64 Pndms;
		INT64 MaxStep;
		float SpecMin;
		float SpecMax;
		float Scale;
	};

#ifdef _MSC_VER
#pragma pack(pop)
#else
#pragma pack()
#endif
	typedef void* FloatVector, * DoubleDimsFloatVector, * Int16Vector, * UInt64Vector, * MelType, * SliceType, * SlicesType, * SvcModel, * VocoderModel;
	typedef const void* CFloatVector, * CDoubleDimsFloatVector, * CInt16Vector, * CUInt64Vector, * CMelType, * CSliceType, * CSlicesType;

	LibSvcApi void InitLibSvcHparams(LibSvcHparams* _Input);

	LibSvcApi void InitLibSvcParams(LibSvcParams* _Input);

	LibSvcApi void InitLibSvcSlicerSettings(LibSvcSlicerSettings* _Input);

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

	LibSvcApi void LibSvcFreeString(
		BSTR _String
	);

	LibSvcApi INT32 LibSvcSetGlobalEnv(
		UINT32 ThreadCount,
		UINT32 DeviceID,
		UINT32 Provider
	);

	LibSvcApi void LibSvcSetMaxErrorCount(
		size_t Count
	);

	LibSvcApi BSTR LibSvcGetError(
		size_t Index
	);

	LibSvcApi INT32 LibSvcSliceAudio(
		CInt16Vector _Audio,						//std::vector<int16_t> By "LibSvcAllocateAudio()"
		const void* _Setting,						//Ptr Of LibSvcSlicerSettings
		UInt64Vector _Output						//std::vector<size_t> By "LibSvcAllocateOffset()"
	);

	LibSvcApi INT32 LibSvcPreprocess(
		CInt16Vector _Audio,						//std::vector<int16_t> By "LibSvcAllocateAudio()"
		CUInt64Vector _SlicePos,					//std::vector<size_t> By "LibSvcAllocateOffset()"
		INT32 _SamplingRate,
		INT32 _HopSize,
		double _Threshold,
		const wchar_t* _F0Method,					//"Dio" "Harvest" "RMVPE" "FCPE"
		SlicesType _Output							//Slices By "LibSvcAllocateSliceData()"
	);

	LibSvcApi INT32 LibSvcStft(
		CInt16Vector _Audio,						//std::vector<int16_t> By "LibSvcAllocateAudio()"
		INT32 _SamplingRate,
		INT32 _Hopsize,
		INT32 _MelBins,
		MelType _Output								//Mel By "LibSvcAllocateMel()"
	);

	LibSvcApi INT32 LibSvcInferSlice(
		SvcModel _Model,							//SingingVoiceConversion Model
		UINT32 _T,
		CSliceType _Slice,							//Slices By "LibSvcAllocateSliceData()"
		const void* _InferParams,					//Ptr Of LibSvcParams
		size_t* _Process, 
		Int16Vector _Output							//std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi INT32 LibSvcInferPCMData(
		SvcModel _Model,							//SingingVoiceConversion Model
		UINT32 _T,
		CInt16Vector _PCMData,
		const void* _InferParams,					//Ptr Of LibSvcParams
		Int16Vector _Output							//std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi LibSvcDeprecated INT32 LibSvcShallowDiffusionInference(
		SvcModel _Model,							//SingingVoiceConversion Model
		CInt16Vector _16KAudioHubert,				//SamplingRate Must Be 16000
		MelType _Mel,								//Mel By "LibSvcAllocateMel()"
		CFloatVector _SrcF0, 
		CFloatVector _SrcVolume, 
		CDoubleDimsFloatVector _SrcSpeakerMap, 
		INT64 _SrcSize,
		const void* _InferParams,					//Ptr Of LibSvcParams
		size_t* _Process,
		Int16Vector _Output							//std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi LibSvcDeprecated INT32 LibSvcVocoderEnhance(
		VocoderModel _Model,						//Vocoder Model
		MelType _Mel,								//Mel By "LibSvcAllocateMel()"
		CFloatVector _F0,
		INT32 _VocoderMelBins,
		Int16Vector _Output							//std::vector<int16_t> By "LibSvcAllocateAudio()"
	);

	LibSvcApi SvcModel LibSvcLoadModel(
		UINT32 _T,
		const void* _Config,						//Ptr Of LibSvcParams
		ProgCallback _ProgressCallback,
		UINT32 _ExecutionProvider = CPU,
		UINT32 _DeviceID = 0,
		UINT32 _ThreadCount = 0
	);

	LibSvcApi INT32 LibSvcUnloadModel(
		UINT32 _T,
		SvcModel _Model
	);

	LibSvcApi VocoderModel LibSvcLoadVocoder(
		LPWSTR VocoderPath
	);

	LibSvcApi INT32 LibSvcUnloadVocoder(
		VocoderModel _Model
	);

	LibSvcApi INT32 LibSvcReadAudio(
		LPWSTR _AudioPath, 
		INT32 _SamplingRate, 
		Int16Vector _Output
	);

	LibSvcApi void LibSvcEnableFileLogger(
		bool _Cond
	);

	LibSvcApi void LibSvcWriteAudioFile(
		Int16Vector _PCMData, 
		LPWSTR _OutputPath, 
		INT32 _SamplingRate
	);

#ifdef __cplusplus
}
#endif