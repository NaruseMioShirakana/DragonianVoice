/**
 * FileName: MoeVSSamplers.hpp
 * Note: MoeVoiceStudioCore Diffusion 官方采样器
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

class ReflowEularSampler : public MoeVSReflowBaseSampler
{
public:
	ReflowEularSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	~ReflowEularSampler() override = default;
	std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process) override;
private:
	const std::vector<const char*> velocityInput = { "x", "t", "cond" };
	const std::vector<const char*> velocityOutput = { "o" };
};

class ReflowRk4Sampler : public MoeVSReflowBaseSampler
{
public:
	ReflowRk4Sampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	~ReflowRk4Sampler() override = default;
	std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process) override;
private:
	const std::vector<const char*> velocityInput = { "x", "t", "cond" };
	const std::vector<const char*> velocityOutput = { "o" };
};

class ReflowHeunSampler : public MoeVSReflowBaseSampler
{
public:
	ReflowHeunSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	~ReflowHeunSampler() override = default;
	std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process) override;
private:
	const std::vector<const char*> velocityInput = { "x", "t", "cond" };
	const std::vector<const char*> velocityOutput = { "o" };
};

class ReflowPececeSampler : public MoeVSReflowBaseSampler
{
public:
	ReflowPececeSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);
	~ReflowPececeSampler() override = default;
	std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process) override;
private:
	const std::vector<const char*> velocityInput = { "x", "t", "cond" };
	const std::vector<const char*> velocityOutput = { "o" };
};

MoeVoiceStudioSamplerEnd