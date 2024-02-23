/**
 * FileName: MoeVSIndexCluster.hpp
 * Note: MoeVoiceStudioCore 官方聚类（Index）
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
#ifdef MoeVoiceStudioIndexCluster
#include <string>
#include "MoeVSBaseCluster.hpp"
#include <faiss/IndexIVFFlat.h>
#include <faiss/index_io.h>
#ifdef NDEBUG
#pragma comment (lib,"../../../../../Lib/faiss/out/build/x64-Release/faiss/faiss.lib")
#endif
#ifdef _DEBUG
#pragma comment (lib,"../../../../../Lib/faiss/out/build/x64-Debug/faiss/faiss.lib")
#endif
#pragma comment (lib,"../../../../../Lib/faiss/faiss/libblas.lib")
#pragma comment (lib,"../../../../../Lib/faiss/faiss/liblapack.lib")
#pragma comment (lib,"../../../../../Lib/faiss/faiss/liblapacke.lib")

MoeVoiceStudioClusterHeader
class IndexClusterCore
{
public:
	IndexClusterCore() = delete;
	~IndexClusterCore();
	IndexClusterCore(const char* _path);
	IndexClusterCore(const IndexClusterCore&) = delete;
	IndexClusterCore(IndexClusterCore&& move) noexcept;
	IndexClusterCore& operator=(const IndexClusterCore&) = delete;
	IndexClusterCore& operator=(IndexClusterCore&& move) noexcept;
	std::vector<float> find(const float* points, faiss::idx_t n_points, faiss::idx_t n_searched_points = 8);
	float* GetVec(faiss::idx_t index);
private:
	faiss::Index* IndexPtr = nullptr;
	faiss::idx_t Dim = 0;
	std::vector<float> IndexsVector;
};

class IndexCluster : public MoeVoiceStudioBaseCluster
{
public:
	IndexCluster() = delete;
	~IndexCluster() override = default;
	IndexCluster(const std::wstring& _path, size_t hidden_size, size_t KmeansLen);
	std::vector<float> find(float* point, long sid, int64_t n_points = 1) override;
private:
	std::vector<IndexClusterCore> Indexs;
	size_t n_hidden_size = 256;
};

MoeVoiceStudioClusterEnd

#endif