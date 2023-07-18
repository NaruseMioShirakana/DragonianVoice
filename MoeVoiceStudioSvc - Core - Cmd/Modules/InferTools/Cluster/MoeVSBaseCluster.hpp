/**
 * FileName: MoeVSBaseCluster.hpp
 * Note: MoeVoiceStudioCore 聚类基类
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
#include <vector>

#define MoeVoiceStudioClusterHeader namespace MoeVoiceStudioCluster {
#define MoeVoiceStudioClusterEnd }

MoeVoiceStudioClusterHeader
	class MoeVoiceStudioBaseCluster
{
public:
	MoeVoiceStudioBaseCluster() = default;
	virtual ~MoeVoiceStudioBaseCluster() = default;

	/**
	 * \brief 查找聚类最邻近点
	 * \param point 待查找的点
	 * \param chara 角色ID
	 * \return 查找到的最邻近点
	 */
	virtual std::vector<float> find(const std::vector<float>& point, long chara);
};

MoeVoiceStudioClusterEnd