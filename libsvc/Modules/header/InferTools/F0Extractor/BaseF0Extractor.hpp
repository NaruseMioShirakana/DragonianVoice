/**
 * FileName: BaseF0Extractor.hpp
 * Note: MoeVoiceStudioCore F0提取算法基类
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
#include <cstdint>
#include <vector>
#include <string>
#define MoeVoiceStudioF0ExtractorHeader namespace MoeVSF0Extractor{
#define MoeVoiceStudioF0ExtractorEnd }

MoeVoiceStudioF0ExtractorHeader
#define __NAME__MOEVS(x) std::wstring ClassName = (x)
class BaseF0Extractor
{
public:
	__NAME__MOEVS(L"MoeVSF0Extractor");

	BaseF0Extractor() = delete;

	/**
	 * \brief 构造F0提取器
	 * \param sampling_rate 采样率
	 * \param hop_size HopSize
	 * \param n_f0_bins F0Bins
	 * \param max_f0 最大F0
	 * \param min_f0 最小F0
	 */
	BaseF0Extractor(int sampling_rate, int hop_size, int n_f0_bins = 256, double max_f0 = 1100.0, double min_f0 = 50.0);

	virtual ~BaseF0Extractor() = default;

	/**
	 * \brief 提取F0
	 * \param PCMData 音频PCM数据（SignedInt16 单声道） 
	 * \param TargetLength 目标F0长度
	 * \return F0
	 */
	virtual std::vector<float> ExtractF0(const std::vector<double>& PCMData, size_t TargetLength);

	/**
	 * \brief 提取F0
	 * \param PCMData 音频PCM数据（SignedInt16 单声道）
	 * \param TargetLength 目标F0长度
	 * \return F0
	 */
	std::vector<float> ExtractF0(const std::vector<float>& PCMData, size_t TargetLength);

	/**
	 * \brief 提取F0
	 * \param PCMData 音频PCM数据（SignedInt16 单声道）
	 * \param TargetLength 目标F0长度
	 * \return F0
	 */
	std::vector<float> ExtractF0(const std::vector<int16_t>& PCMData, size_t TargetLength);

	static std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0);
protected:
	const uint32_t fs;
	const uint32_t hop;
	const uint32_t f0_bin;
	const double f0_max;
	const double f0_min;
	double f0_mel_min;
	double f0_mel_max;
};
#undef __NAME__MOEVS
MoeVoiceStudioF0ExtractorEnd