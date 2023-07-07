#pragma once
#include "MoeVSBaseSampler.hpp"

MoeVoiceStudioSamplerHeader

class PndmSampler : public MoeVSBaseSampler
{
public:
	PndmSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	~PndmSampler() override = default;
	std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process) override;
private:
	const std::vector<const char*> denoiseInput = { "noise", "time", "condition" };
	const std::vector<const char*> predInput = { "noise", "noise_pred", "time", "time_prev" };
	const std::vector<const char*> denoiseOutput = { "noise_pred" };
	const std::vector<const char*> predOutput = { "noise_pred_o" };
};

class DDimSampler : public MoeVSBaseSampler
{
public:
	DDimSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	~DDimSampler() override = default;
	std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process) override;
private:
	const std::vector<const char*> alphain = { "time" };
	const std::vector<const char*> alphaout = { "alphas_cumprod" };
	const std::vector<const char*> denoiseInput = { "noise", "time", "condition" };
	const std::vector<const char*> denoiseOutput = { "noise_pred" };
};

MoeVoiceStudioSamplerEnd