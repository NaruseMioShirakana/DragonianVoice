/**
 * FileName: TensorExtractorManager.hpp
 * Note: MoeVoiceStudioCore 张量预处理类的注册和管理
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
#include <functional>
#include "MoeVoiceStudioTensorExtractor.hpp"

MoeVoiceStudioTensorExtractorHeader

class TensorExtractor
{
public:
	TensorExtractor() = default;
	TensorExtractor(MoeVoiceStudioTensorExtractor* _ext) : _f0_ext(_ext) {}
	TensorExtractor(const TensorExtractor&) = delete;
	TensorExtractor(TensorExtractor&& _ext) noexcept
	{
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
	}
	TensorExtractor& operator=(const TensorExtractor&) = delete;
	TensorExtractor& operator=(TensorExtractor&& _ext) noexcept
	{
		if (this == &_ext)
			return *this;
		delete _f0_ext;
		_f0_ext = _ext._f0_ext;
		_ext._f0_ext = nullptr;
		return *this;
	}
	~TensorExtractor()
	{
		delete _f0_ext;
		_f0_ext = nullptr;
	}
	MoeVoiceStudioTensorExtractor* operator->() const { return _f0_ext; }

private:
	MoeVoiceStudioTensorExtractor* _f0_ext = nullptr;
};

using GetTensorExtractorFn = std::function<TensorExtractor(uint64_t, uint64_t, uint64_t, bool, bool, uint64_t, uint64_t, const MoeVoiceStudioTensorExtractor::Others&)>;

void RegisterTensorExtractor(const std::wstring& _name, const GetTensorExtractorFn& _constructor_fn);

/**
 * \brief 获取张量预处理器
 * \param _name 类名
 * \param _srcsr 原始采样率
 * \param _sr 目标采样率
 * \param _hop HopSize
 * \param _smix 是否启用角色混合
 * \param _volume 是否启用音量emb
 * \param _hidden_size hubert的维数
 * \param _nspeaker 角色数
 * \param _other 其他参数，其中的memoryInfo必须为你当前模型的memoryInfo
 * \return 张量预处理器
 */
TensorExtractor GetTensorExtractor(
	const std::wstring& _name,
	uint64_t _srcsr, uint64_t _sr, uint64_t _hop,
	bool _smix, bool _volume, uint64_t _hidden_size,
	uint64_t _nspeaker,
	const MoeVoiceStudioTensorExtractor::Others& _other
);

MoeVoiceStudioTensorExtractorEnd