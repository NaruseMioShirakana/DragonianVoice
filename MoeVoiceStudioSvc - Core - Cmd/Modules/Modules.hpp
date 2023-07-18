/**
 * FileName: Modules.hpp
 * Note: MoeVoiceStudioCore组件管理
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
#include "Models/header/VitsSvc.hpp"
#include "Models/header/DiffSvc.hpp"

namespace MoeVSModuleManager
{
	inline int64_t SamplingRate = 32000;
	inline int64_t SpeakerCount = 0;

	/**
	 * \brief 初始化所有组件
	 */
	void MoeVoiceStudioCoreInitSetup();

	/**
	 * \brief 获取当前模型
	 * \return 当前模型的指针
	 */
	MoeVoiceStudioCore::SingingVoiceConversion* GetCurSvcModel();

	/**
	 * \brief 卸载模型
	 */
	void UnloadSvcModel();

	/**
	 * \brief 载入模型
	 * \param Config 一个MJson类的实例（配置文件的JSON）
	 * \param Callback 进度条回调函数
	 * \param ProviderID Provider在所有Provider中的ID（遵循Enum Class的定义）
	 * \param NumThread CPU推理时的线程数（最好设置高一点，GPU不支持的算子可能也会Fallback到CPU）
	 * \param DeviceID GPU设备ID
	 */
	void LoadSvcModel(const MJson& Config,
		const MoeVoiceStudioCore::MoeVoiceStudioModule::ProgressCallback& Callback,
		int ProviderID, int NumThread, int DeviceID);
}

namespace MoeVSRename
{
	using MoeVSVitsBasedSvc = MoeVoiceStudioCore::VitsSvc;
	using MoeVSDiffBasedSvc = MoeVoiceStudioCore::DiffusionSvc;
}

