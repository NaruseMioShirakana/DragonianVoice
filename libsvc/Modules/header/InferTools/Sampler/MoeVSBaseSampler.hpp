/**
 * FileName: MoeVSBaseSampler.hpp
 * Note: MoeVoiceStudioCore Diffusion 采样器基类
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
#define MoeVoiceStudioSamplerHeader namespace MoeVSSampler {
#define MoeVoiceStudioSamplerEnd }
#include <functional>
#include <onnxruntime_cxx_api.h>
MoeVoiceStudioSamplerHeader

class MoeVSBaseSampler
{
public:
	using ProgressCallback = std::function<void(size_t, size_t)>;

	/**
	 * \brief 构造采样器
	 * \param alpha Alphas Onnx模型Session
	 * \param dfn DenoiseFn Onnx模型Session
	 * \param pred Predictor Onnx模型Session
	 * \param Mel_Bins MelBins
	 * \param _ProgressCallback 进度条回调（直接传模型的回调就可以了） 
	 * \param memory 模型的OrtMemoryInfo
	 */
	MoeVSBaseSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);

	virtual ~MoeVSBaseSampler() = default;

	/**
	 * \brief 采样
	 * \param Tensors 输入张量（Tensors[0]为Condition，Tensors[1]为初始噪声）
	 * \param Steps 采样步数
	 * \param SpeedUp 加速倍数
	 * \param NoiseScale 噪声规模
	 * \param Seed 种子
	 * \param Process 当前进度
	 * \return Mel张量
	 */
	virtual std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process);
protected:
	int64_t MelBins = 128;
	Ort::Session* Alpha = nullptr;
	Ort::Session* DenoiseFn = nullptr;
	Ort::Session* NoisePredictor = nullptr;
	ProgressCallback _callback;
	Ort::MemoryInfo* Memory = nullptr;
};

class MoeVSReflowBaseSampler
{
public:
	using ProgressCallback = std::function<void(size_t, size_t)>;

	/**
	 * \brief 获取采样器
	 * \param Velocity Velocity Onnx模型Session
	 * \param MelBins MelBins
	 * \param _ProgressCallback 进度条回调（直接传模型的回调就可以了）
	 * \param memory 模型的OrtMemoryInfo
	 * \return 采样器
	 */
	MoeVSReflowBaseSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory);

	virtual ~MoeVSReflowBaseSampler() = default;

	/**
	 * \brief 采样
	 * \param Tensors 输入张量
	 * \param Steps 采样步数
	 * \param dt dt
	 * \param Scale Scale
	 * \param Process 当前进度
	 * \return Mel张量
	 */
	virtual std::vector<Ort::Value> Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process);
protected:
	int64_t MelBins_ = 128;
	Ort::Session* Velocity_ = nullptr;
	ProgressCallback Callback_;
	Ort::MemoryInfo* Memory_ = nullptr;
};

MoeVoiceStudioSamplerEnd