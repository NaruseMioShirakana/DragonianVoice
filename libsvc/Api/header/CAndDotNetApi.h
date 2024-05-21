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

	LibSvcApi float* LibSvcGetFloatVectorData(void* _Obj);

	LibSvcApi size_t LibSvcGetFloatVectorSize(void* _Obj);

	//DFloatVector

	LibSvcApi void* LibSvcGetDFloatVectorData(void* _Obj, size_t _Index);

	LibSvcApi size_t LibSvcGetDFloatVectorSize(void* _Obj);

	//Audio - vector<int16_t>

	LibSvcApi void* LibSvcAllocateAudio();

	LibSvcApi void LibSvcReleaseAudio(void* _Obj);

	LibSvcApi void LibSvcSetAudioLength(void* _Obj, size_t _Size);

	LibSvcApi short* LibSvcGetAudioData(void* _Obj);

	LibSvcApi size_t LibSvcGetAudioSize(void* _Obj);

	//Offset - vector<size_t>

	LibSvcApi void* LibSvcAllocateOffset();

	LibSvcApi void LibSvcReleaseOffset(void* _Obj);

	LibSvcApi size_t* LibSvcGetOffsetData(void* _Obj);

	LibSvcApi size_t LibSvcGetOffsetSize(void* _Obj);

	//Mel - pair<vector<float>, int64_t>

	LibSvcApi void* LibSvcAllocateMel();

	LibSvcApi void LibSvcReleaseMel(void* _Obj);

	LibSvcApi void* LibSvcGetMelData(void* _Obj);

	LibSvcApi INT64 LibSvcGetMelSize(void* _Obj);

	//Slice - MoeVoiceStudioSvcSlice

	LibSvcApi void* LibSvcGetAudio(void* _Obj);

	LibSvcApi void* LibSvcGetF0(void* _Obj);

	LibSvcApi void* LibSvcGetVolume(void* _Obj);

	LibSvcApi void* LibSvcGetSpeaker(void* _Obj);

	LibSvcApi INT32 LibSvcGetSrcLength(void* _Obj);

	LibSvcApi INT32 LibSvcGetIsNotMute(void* _Obj);

	LibSvcApi void LibSvcSetSpeakerMixDataSize(void* _Obj, size_t _NSpeaker);

	//Slices - MoeVoiceStudioSvcData

	LibSvcApi void* LibSvcAllocateSliceData();

	LibSvcApi void LibSvcReleaseSliceData(void* _Obj);

	LibSvcApi BSTR LibSvcGetAudioPath(void* _Obj);

	LibSvcApi void* LibSvcGetSlice(void* _Obj, size_t _Index);

	LibSvcApi size_t LibSvcGetSliceCount(void* _Obj);

	/******************************************Fun**********************************************/

	LibSvcApi void LibSvcInit();

	LibSvcApi INT32 LibSvcSetGlobalEnv(UINT32 ThreadCount, UINT32 DeviceID, UINT32 Provider);

	LibSvcApi void LibSvcSetMaxErrorCount(size_t Count);

	LibSvcApi BSTR LibSvcGetError(size_t Index);

	LibSvcApi INT32 LibSvcSliceAudio(
		const void* _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
		const void* _Setting, //LibSvcSlicerSettings
		void* _Output //std::vector<size_t> By "LibSvcAllocateOffset()"
	);

	LibSvcApi INT32 LibSvcPreprocess(
		const void* _Audio, //std::vector<int16_t> By "LibSvcAllocateAudio()"
		const void* _SlicePos, //std::vector<size_t> By "LibSvcAllocateOffset()"
		INT32 _SamplingRate,
		INT32 _HopSize,
		double _Threshold,
		const wchar_t* _F0Method,
		void* _Output // Slices By "LibSvcAllocateSliceData()"
	);

	LibSvcApi INT32 LibSvcStft(
		const void* _Audio,
		INT32 _SamplingRate,
		INT32 _Hopsize,
		INT32 _MelBins,
		void* _Output
	);

	LibSvcApi INT32 LibSvcInferSlice(
		void* _Model,
		UINT32 _T,
		const void* _Slice,
		const void* _InferParams,
		size_t* _Process,
		void* _Output
	);

	LibSvcApi INT32 LibSvcShallowDiffusionInference(
		void* _Model,
		void* _16KAudioHubert,
		void* _Mel,
		const void* _SrcF0,
		const void* _SrcVolume,
		const void* _SrcSpeakerMap,
		INT64 _SrcSize,
		const void* _InferParams,
		size_t* _Process,
		void* _Output
	);

	LibSvcApi INT32 LibSvcVocoderEnhance(
		void* _Model,
		void* _Mel,
		void* _F0,
		INT32 _VocoderMelBins,
		void* _Output
	);

	LibSvcApi void* LibSvcLoadModel(
		UINT32 _T,
		const void* _Config,
		ProgCallback _ProgressCallback,
		UINT32 _ExecutionProvider = CPU,
		UINT32 _DeviceID = 0,
		UINT32 _ThreadCount = 0
	);

	LibSvcApi INT32 LibSvcUnloadModel(
		UINT32 _T,
		void* _Model
	);

	LibSvcApi void* LibSvcLoadVocoder(LPWSTR VocoderPath);

	LibSvcApi INT32 LibSvcUnloadVocoder(void* _Model);

	LibSvcApi INT32 LibSvcReadAudio(LPWSTR _AudioPath, INT32 _SamplingRate, void* _Output);

#ifdef __cplusplus
}
#endif