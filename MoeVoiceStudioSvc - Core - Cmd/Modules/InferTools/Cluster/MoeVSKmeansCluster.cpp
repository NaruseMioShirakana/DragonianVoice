#include "MoeVSKmeansCluster.hpp"
#include "../inferTools.hpp"

std::vector<float> MoeVoiceStudioCluster::KMeansCluster::find(float* point, long sid, int64_t n_points)
{
	if (size_t(sid) < _tree.size())
	{
		std::vector<float> res;
		res.reserve(dims * n_points * 2);
		for (int64_t pt = 0; pt < n_points; ++pt)
		{
			auto tmp = _tree[sid].nearest_point({ point + pt * dims,point + (pt + 1) * dims });
			res.insert(res.end(), tmp.begin(), tmp.end());
		}
		return res;
	}
	return { point, point + dims * n_points };
}

MoeVoiceStudioCluster::KMeansCluster::KMeansCluster(const std::wstring& _path, size_t hidden_size, size_t KmeansLen)
{
	dims = hidden_size;
	FILE* file = nullptr;
	_wfopen_s(&file, (_path + L"/KMeans.npy").c_str(), L"rb");
	if (!file)
		LibDLVoiceCodecThrow("KMeansFileNotExist");
	constexpr long idx = 128;
	fseek(file, idx, SEEK_SET);
	std::vector<float> tmpData(hidden_size);
	const size_t ec = size_t(hidden_size) * sizeof(float);
	std::vector<std::vector<float>> _tmp;
	_tmp.reserve(KmeansLen);
	while (fread(tmpData.data(), 1, ec, file) == ec)
	{
		_tmp.emplace_back(tmpData);
		if (_tmp.size() == KmeansLen)
		{
			_tree.emplace_back(_tmp);
			_tmp.clear();
		}
	}
}
