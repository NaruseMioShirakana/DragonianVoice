#pragma once
#include "MoeVoiceStudioTensorExtractor.hpp"

MoeVoiceStudioTensorExtractorHeader

class SoVits2TensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	SoVits2TensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~SoVits2TensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;

	const std::vector<const char*> InputNames = { "hidden_unit", "lengths", "pitch", "sid" };
};

class SoVits3TensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	SoVits3TensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~SoVits3TensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;

	const std::vector<const char*> InputNames = { "hidden_unit", "lengths", "pitch", "sid" };
};

class SoVits4TensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	SoVits4TensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~SoVits4TensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;

	const std::vector<const char*> InputNames = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
	const std::vector<const char*> InputNamesVol = { "c", "f0", "mel2ph", "uv", "noise", "sid", "vol" };
};

class SoVits4DDSPTensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	SoVits4DDSPTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~SoVits4DDSPTensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;

	const std::vector<const char*> InputNames = { "c", "f0", "mel2ph", "t_window", "noise", "sid" };
	const std::vector<const char*> InputNamesVol = { "c", "f0", "mel2ph", "t_window", "noise", "sid", "vol" };
};

class RVCTensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	RVCTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~RVCTensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;

	const std::vector<const char*> InputNames = { "phone", "phone_lengths", "pitch", "pitchf", "ds", "rnd" };
	const std::vector<const char*> InputNamesVol = { "phone", "phone_lengths", "pitch", "pitchf", "ds", "rnd", "vol" };
};

class DiffSvcTensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	DiffSvcTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~DiffSvcTensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;
	const std::vector<const char*> InputNames = { "hubert", "mel2ph", "spk_embed", "f0" };
	const std::vector<const char*> OutputNames = { "mel_pred", "f0_pred" };
};

class DiffusionSvcTensorExtractor : public MoeVoiceStudioTensorExtractor
{
public:
	DiffusionSvcTensorExtractor(uint64_t _srcsr, uint64_t _sr, uint64_t _hop, bool _smix, bool _volume, uint64_t _hidden_size, uint64_t _nspeaker, const Others& _other) : MoeVoiceStudioTensorExtractor(_srcsr, _sr, _hop, _smix, _volume, _hidden_size, _nspeaker, _other) {}
	~DiffusionSvcTensorExtractor() override = default;
	Inputs Extract(
		const std::vector<float>& HiddenUnit,
		const std::vector<float>& F0,
		const std::vector<float>& Volume,
		const std::vector<std::vector<float>>& SpkMap,
		Params params
	) override;
	const std::vector<const char*> InputNamesVol = { "hubert", "mel2ph", "f0", "volume", "spk_mix" };
	const std::vector<const char*> InputNames = { "hubert", "mel2ph", "f0", "spk_mix" };
	const std::vector<const char*> OutputNames = { "mel_pred" };
};

MoeVoiceStudioTensorExtractorEnd