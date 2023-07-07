#pragma once
#include "MoeVSBaseCluster.hpp"
#include "../../DataStruct/KDTree.hpp"
#include <string>

MoeVoiceStudioClusterHeader

class KMeansCluster : public MoeVoiceStudioBaseCluster
{
public:
	KMeansCluster() = default;
	~KMeansCluster() override = default;
	KMeansCluster(const std::wstring& _path, size_t hidden_size, size_t KmeansLen);
	std::vector<float> find(const std::vector<float>& point, long chara) override;
private:
	std::vector<KDTree> _tree;
};

MoeVoiceStudioClusterEnd