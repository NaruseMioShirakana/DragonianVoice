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

MoeVSCluster GetMoeVSCluster(const std::wstring& _name, const std::wstring& _path, size_t hidden_size, size_t KmeansLen);

MoeVoiceStudioClusterEnd