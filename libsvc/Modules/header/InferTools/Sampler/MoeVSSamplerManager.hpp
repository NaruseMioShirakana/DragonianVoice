/**
 * FileName: MoeVSSamplerManager.hpp
 * Note: MoeVoiceStudioCore Diffusion 采样器管理
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
#include "../../framework.h"

MoeVoiceStudioSamplerHeader

class MoeVSSampler
{
public:
	MoeVSSampler() = delete;
	MoeVSSampler(MoeVSBaseSampler* _ext) : _f0_ext(_ext) {}
	MoeVSSampler(const MoeVSSampler&) = delete;
	MoeVSSampler(MoeVSSampler&& _ext) noexcept
	{
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
	}
	MoeVSSampler& operator=(const MoeVSSampler&) = delete;
	MoeVSSampler& operator=(MoeVSSampler&& _ext) noexcept
	{
		if (this == &_ext)
			return *this;
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
		return *this;
	}
	~MoeVSSampler()
	{
		delete _f0_ext;
		_f0_ext = nullptr;
	}
	MoeVSBaseSampler* operator->() const { return _f0_ext; }

private:
	MoeVSBaseSampler* _f0_ext = nullptr;
};

using GetMoeVSSamplerFn = std::function<MoeVSSampler(Ort::Session*, Ort::Session*, Ort::Session*, int64_t, const MoeVSBaseSampler::ProgressCallback&, Ort::MemoryInfo*)>;

LibSvcApi void RegisterMoeVSSampler(const std::wstring& _name, const GetMoeVSSamplerFn& _constructor_fn);

/**
 * \brief 获取采样器
 * \param _name 类名
 * \param alpha Alphas Onnx模型Session
 * \param dfn DenoiseFn Onnx模型Session
 * \param pred Predictor Onnx模型Session
 * \param Mel_Bins MelBins
 * \param _ProgressCallback 进度条回调（直接传模型的回调就可以了）
 * \param memory 模型的OrtMemoryInfo
 * \return 采样器
 */
LibSvcApi MoeVSSampler GetMoeVSSampler(const std::wstring& _name,
                             Ort::Session* alpha,
                             Ort::Session* dfn,
                             Ort::Session* pred,
                             int64_t Mel_Bins,
                             const MoeVSBaseSampler::ProgressCallback& _ProgressCallback,
                             Ort::MemoryInfo* memory);

LibSvcApi std::vector<std::wstring> GetMoeVSSamplerList();

MoeVoiceStudioSamplerEnd