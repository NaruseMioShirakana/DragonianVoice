#include "MoeVSKmeansCluster.hpp"

std::vector<float> MoeVoiceStudioCluster::KMeansCluster::find(const std::vector<float>& point, long chara)
{
	auto res = _tree[chara].nearest_point(point);
	return res;
}

MoeVoiceStudioCluster::KMeansCluster::KMeansCluster(const std::wstring& _path, size_t hidden_size, size_t KmeansLen)
{
	FILE* file = nullptr;
	_wfopen_s(&file, _path.c_str(), L"rb");
	if (!file)
		throw std::exception("KMeansFileNotExist");
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
