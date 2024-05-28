#include "MoeVSClusterManager.hpp"
#include <map>
#include <stdexcept>
#include "../../Logger/MoeSSLogger.hpp"

MoeVoiceStudioClusterHeader

std::map<std::wstring, GetMoeVSClusterFn> RegisteredMoeVSCluster;

MoeVSCluster GetMoeVSCluster(const std::wstring& _name, const std::wstring& _path, size_t hidden_size, size_t KmeansLen)
{
	const auto f_ClusterFn = RegisteredMoeVSCluster.find(_name);
	if (f_ClusterFn != RegisteredMoeVSCluster.end())
		return f_ClusterFn->second(_path, hidden_size, KmeansLen);
	throw std::runtime_error("Unable To Find An Available MoeVSCluster");
}

void RegisterMoeVSCluster(const std::wstring& _name, const GetMoeVSClusterFn& _constructor_fn)
{
	if (RegisteredMoeVSCluster.find(_name) != RegisteredMoeVSCluster.end())
	{
		logger.log(L"[Warn] MoeVSClusterNameConflict");
		return;
	}
	RegisteredMoeVSCluster[_name] = _constructor_fn;
}

MoeVoiceStudioClusterEnd