/**
 * FileName: MoeVSClusterManager.hpp
 * Note: MoeVoiceStudioCore 聚类管理
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
#include "MoeVSBaseCluster.hpp"
#include <functional>
#include <string>

MoeVoiceStudioClusterHeader
class MoeVSCluster
{
public:
	MoeVSCluster() = default;
	MoeVSCluster(MoeVoiceStudioBaseCluster* _ext) : _cluster_ext(_ext) {}
	MoeVSCluster(const MoeVSCluster&) = delete;
	MoeVSCluster(MoeVSCluster&& _ext) noexcept
	{
		delete _cluster_ext;
		_cluster_ext = _ext._cluster_ext;
		_ext._cluster_ext = nullptr;
	}
	MoeVSCluster& operator=(const MoeVSCluster&) = delete;
	MoeVSCluster& operator=(MoeVSCluster&& _ext) noexcept
	{
		if (this == &_ext)
			return *this;
		delete _cluster_ext;
		_cluster_ext = _ext._cluster_ext;
		_ext._cluster_ext = nullptr;
		return *this;
	}
	~MoeVSCluster()
	{
		delete _cluster_ext;
		_cluster_ext = nullptr;
	}
	MoeVoiceStudioBaseCluster* operator->() const { return _cluster_ext; }
private:
	MoeVoiceStudioBaseCluster* _cluster_ext = nullptr;
};

using GetMoeVSClusterFn = std::function<MoeVSCluster(const std::wstring&, size_t, size_t)>;

void RegisterMoeVSCluster(const std::wstring& _name, const GetMoeVSClusterFn& _constructor_fn);

/**
 * \brief 获取聚类
 * \param _name 类名
 * \param _path 聚类数据路径
 * \param hidden_size hubert维数
 * \param KmeansLen 聚类的长度
 * \return 聚类
 */
MoeVSCluster GetMoeVSCluster(const std::wstring& _name, const std::wstring& _path, size_t hidden_size, size_t KmeansLen);

MoeVoiceStudioClusterEnd