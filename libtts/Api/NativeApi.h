#pragma once
#include "../../framework.h"
#include "windows.h"
#ifdef __cplusplus
extern "C" {
#endif

	typedef void(*LibTTSProgCallback)(size_t, size_t);

	enum LibTTSExecutionProviders { CPU = 0, CUDA = 1, DML = 2 };

	enum LibTTSModelType { Vits, GptSoVits };

#ifdef _MSC_VER
#pragma pack(push, 4)
#else
#pragma pack(4)
#endif

	struct LibTTSParams;
	struct LibTTSToken;
	struct LibTTSSeq;

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

	struct LibTTSHparams
	{
		LPWSTR TensorExtractor = nullptr;
		DiffusionSvcPaths DiffusionSvc;
		ReflowSvcPaths ReflowSvc;

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

	/******************************************Fun**********************************************/

	LibTTSApi void LibTTSInit();

	LibTTSApi INT32 LibTTSSetGlobalEnv(UINT32 ThreadCount, UINT32 DeviceID, UINT32 Provider);

	LibTTSApi void LibTTSSetMaxErrorCount(size_t Count);

	LibTTSApi BSTR LibTTSGetError(size_t Index);

	LibTTSApi void* LibTTSLoadModel(
		UINT32 _T,
		const void* _Config,
		LibTTSProgCallback _ProgressCallback,
		UINT32 _ExecutionProvider = CPU,
		UINT32 _DeviceID = 0,
		UINT32 _ThreadCount = 0
	);

	LibTTSApi INT32 LibTTSUnloadModel(
		UINT32 _T,
		void* _Model
	);

	LibTTSApi void* LibTTSLoadVocoder(LPWSTR VocoderPath);

	LibTTSApi INT32 LibTTSUnloadVocoder(void* _Model);

	LibTTSApi void LibTTSEnableFileLogger(bool _Cond);

	LibTTSApi void LibTTSWriteAudioFile(void* _PCMData, LPWSTR _OutputPath, INT32 _SamplingRate);

#ifdef __cplusplus
}
#endif