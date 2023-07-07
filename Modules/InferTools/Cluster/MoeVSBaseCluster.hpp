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

	virtual std::vector<float> find(const std::vector<float>& point, long chara);
};

MoeVoiceStudioClusterEnd