#pragma once
#include "Model/ModelStructs.h"

class SkinnedModel;
class Skeleton;
class DirectXManager;
class SrvManager;

class SkinCluster
{
public:
	void Initialize(
		const SkeletonData& skeleton,
		const SkinnedMeshData& meshData, // メッシュごとのデータ
		const std::map<std::string, JointWeightData>& skinClusterData,
		DirectXManager* dxManager,
		SrvManager* srvManager);

	// SkinClusterを生成する関数
	SkinClusterData CreateSkinCluster(
		const SkeletonData& skeleton,
		const SkinnedMeshData& meshData, // メッシュごとのデータ
		const std::map<std::string, JointWeightData>& skinClusterData,
		DirectXManager* dxManager,
		SrvManager* srvManager);
	// スキンクラスターの更新
	void UpdateSkinCluster(const SkeletonData& skeleton);

private:
	// スキンクラスター
	SkinClusterData skinCluster_;

public:
	SkinClusterData GetSkinCluster() const { return skinCluster_; }
};

