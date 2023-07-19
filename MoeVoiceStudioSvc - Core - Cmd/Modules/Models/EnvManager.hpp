#pragma once
#include <onnxruntime_cxx_api.h>

#define MoeVoiceStudioCoreEnvManagerHeader namespace moevsenv{
#define MoeVoiceStudioCoreEnvManagerEnd }

MoeVoiceStudioCoreEnvManagerHeader
class MoeVoiceStudioEnv
{
public:
	MoeVoiceStudioEnv() = default;
	~MoeVoiceStudioEnv() { Destory(); }
	void Load(unsigned ThreadCount, unsigned DeviceID, unsigned Provider);
	void Destory();
	[[nodiscard]] bool IsEnabled() const;
	[[nodiscard]] Ort::Env* GetEnv() const { return GlobalOrtEnv; }
	[[nodiscard]] Ort::SessionOptions* GetSessionOptions() const { return GlobalOrtSessionOptions; }
	[[nodiscard]] Ort::MemoryInfo* GetMemoryInfo() const { return GlobalOrtMemoryInfo; }
private:
	void Create(unsigned ThreadCount_, unsigned DeviceID_, unsigned ExecutionProvider_);
	Ort::Env* GlobalOrtEnv = nullptr;
	Ort::SessionOptions* GlobalOrtSessionOptions = nullptr;
	Ort::MemoryInfo* GlobalOrtMemoryInfo = nullptr;
	unsigned CurThreadCount = unsigned(-1);
	unsigned CurDeviceID = unsigned(-1);
	unsigned CurProvider = unsigned(-1);
};

MoeVoiceStudioEnv& GetGlobalMoeVSEnv();

MoeVoiceStudioCoreEnvManagerEnd