#ifdef MoeVoiceStudioIndexCluster
#include "MoeVSIndexCluster.hpp"
#include <filesystem>
#include "../inferTools.hpp"
MoeVoiceStudioClusterHeader

IndexClusterCore::~IndexClusterCore()
{
	delete IndexPtr;
	IndexPtr = nullptr;
}

IndexClusterCore::IndexClusterCore(const char* _path)
{
	IndexPtr = faiss::read_index(_path);
	IndexsVector = std::vector(IndexPtr->ntotal * IndexPtr->d, 0.f);
	IndexPtr->reconstruct_n(0, IndexPtr->ntotal, IndexsVector.data());
	Dim = IndexPtr->d;
}

IndexClusterCore::IndexClusterCore(IndexClusterCore&& move) noexcept
{
	IndexPtr = move.IndexPtr;
	Dim = move.Dim;
	IndexsVector = std::move(move.IndexsVector);
	move.IndexPtr = nullptr;
}

IndexClusterCore& IndexClusterCore::operator=(IndexClusterCore&& move) noexcept
{
	if (&move == this) return *this;
	delete IndexPtr;
	IndexPtr = move.IndexPtr;
	Dim = move.Dim;
	IndexsVector = std::move(move.IndexsVector);
	move.IndexPtr = nullptr;
	return *this;
}

float* IndexClusterCore::GetVec(faiss::idx_t index)
{
	return IndexsVector.data() + index * Dim;
}

std::vector<float> IndexClusterCore::find(const float* points, faiss::idx_t n_points, faiss::idx_t n_searched_points)
{
	std::vector<float> result(Dim * n_points);
	std::vector<float> distances(n_searched_points * n_points);
	std::vector<faiss::idx_t> labels(n_searched_points * n_points);
	IndexPtr->search(n_points, points, n_searched_points, distances.data(), labels.data());
	for (faiss::idx_t pt = 0; pt < n_points; ++pt)
	{
		std::vector result_pt(Dim, 0.f);                                   
		const auto idx_vec = labels.data() + pt * n_searched_points;     // SIZE:[n_searched_points]
		const auto dis_vec = distances.data() + pt * n_searched_points;  // SIZE:[n_searched_points]
		float sum = 0.f;                                                       // dis_vec[i] / sum = pGetVec(idx_vec[i])
		for (faiss::idx_t spt = 0; spt < n_searched_points; ++spt)             // result_pt = GetVec(idx_vec[i])
		{
			if(idx_vec[spt] < 0)
				continue;
			dis_vec[spt] = (1 / dis_vec[spt]) * (1 / dis_vec[spt]);
			sum += dis_vec[spt];
		}
		if (sum == 0.f) sum = 1.f;
		for (faiss::idx_t spt = 0; spt < n_searched_points; ++spt)
		{
			if (idx_vec[spt] < 0)
				continue;
			const auto sedpt = GetVec(idx_vec[spt]);
			const auto pcnt = (dis_vec[spt] / sum);
			for (faiss::idx_t sptp = 0; sptp < Dim; ++sptp)
				result_pt[sptp] += pcnt * sedpt[sptp];
		}
		memcpy(result.data() + pt * Dim, result_pt.data(), Dim * sizeof(float));
	}
	return result;
}

IndexCluster::IndexCluster(const std::wstring& _path, size_t hidden_size, size_t KmeansLen)
{
	const auto RawPath = _path + L"/Index-";
	size_t idx = 0;
	while(true)
	{
		std::filesystem::path IndexPath = RawPath + std::to_wstring(idx++) + L".index";
		if(!exists(IndexPath)) break;
		Indexs.emplace_back(IndexPath.string().c_str());
	}
	if (Indexs.empty())
		LibDLVoiceCodecThrow("Index Is Empty");
}

std::vector<float> IndexCluster::find(float* point, long sid, int64_t n_points)
{
	if (size_t(sid) < Indexs.size())
		return Indexs[sid].find(point, n_points);
	return { point,point + n_hidden_size * n_points };
}

MoeVoiceStudioClusterEnd
#endif