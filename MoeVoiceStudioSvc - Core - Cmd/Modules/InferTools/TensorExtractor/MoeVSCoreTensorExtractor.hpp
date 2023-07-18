/**
 * FileName: MoeVSCoreTensorExtractor.hpp
 * Note: MoeVoiceStudioCore 官方张量预处理
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2022-10-17 Create
*/

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