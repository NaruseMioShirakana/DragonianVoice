#include "MoeVSBaseSampler.hpp"

MoeVoiceStudioSamplerHeader

MoeVSBaseSampler::MoeVSBaseSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :
	MelBins(Mel_Bins), Alpha(alpha), DenoiseFn(dfn), NoisePredictor(pred)
{
	_callback = _ProgressCallback;
	Memory = memory;
};

std::vector<Ort::Value> MoeVSBaseSampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process)
{
	throw std::exception("NotImplementedError");
}

MoeVoiceStudioSamplerEnd