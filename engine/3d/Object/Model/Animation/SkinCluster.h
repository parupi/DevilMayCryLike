#pragma once
#include <3d/Object/Model/ModelStructs.h>

class SkinnedModel;
class Skeleton;
class DirectXManager;
class SrvManager;

class SkinCluster
{
public:
	SkinCluster() = default;
	~SkinCluster();

	void Initialize(const SkeletonData& skeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData, DirectXManager* dxManager, SrvManager* srvManager);

	// SkinClusterを生成する関数
	void CreateSkinCluster(const SkeletonData& skeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData);

	void UpdateSkinning();

	void UpdateInputVertex(const SkinnedMeshData& modelData);

	void UpdateSkinCluster(const SkeletonData& skeleton);

	// paletteの生成
	void CreatePaletteResource(const SkeletonData& skeleton);
	// influenceの生成
	void CreateInfluenceResource(const SkinnedMeshData& meshData);
	// inputVertexの生成
	void CreateInputVertexResource();
	// outputVertexの生成
	void CreateOutputVertexResource();
	// skinningInfoの生成
	void CreateSkinningInfoResource();

	SkinClusterData& GetSkinCluster() { return skinCluster_; }
private:
	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// スキンクラスター
	SkinClusterData skinCluster_;
	// 頂点数
	uint32_t vertexCount_;
	size_t vertexOffset = 0;
};

