#pragma once
#define MoeVoiceStudioSamplerHeader namespace MoeVSSampler {
#define MoeVoiceStudioSamplerEnd }
#include <functional>
#include <onnxruntime_cxx_api.h>
MoeVoiceStudioSamplerHeader

class MoeVSBaseSampler
{
public:
	using ProgressCallback = std::function<void(size_t, size_t)>;
	MoeVSBaseSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	virtual ~MoeVSBaseSampler() = default;
	virtual std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process);
protected:
	int64_t MelBins = 128;
	Ort::Session* Alpha = nullptr;
	Ort::Session* DenoiseFn = nullptr;
	Ort::Session* NoisePredictor = nullptr;
	ProgressCallback _callback;
	Ort::MemoryInfo* Memory = nullptr;
};

MoeVoiceStudioSamplerEnd