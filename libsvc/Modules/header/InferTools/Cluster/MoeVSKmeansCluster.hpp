/**
 * FileName: MoeVSKmeansCluster.hpp
 * Note: MoeVoiceStudioCore 官方聚类（Kmeans）
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
#include "../DataStruct/KDTree.hpp"
#include <string>

MoeVoiceStudioClusterHeader

class KMeansCluster : public MoeVoiceStudioBaseCluster
{
public:
	KMeansCluster() = delete;
	~KMeansCluster() override = default;
	KMeansCluster(const std::wstring& _path, size_t hidden_size, size_t KmeansLen);
	std::vector<float> find(float* point, long sid, int64_t n_points = 1) override;
private:
	std::vector<KDTree> _tree;
	size_t dims = 0;
};

MoeVoiceStudioClusterEnd