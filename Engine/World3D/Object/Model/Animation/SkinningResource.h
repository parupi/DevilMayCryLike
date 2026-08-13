#pragma once
#include <World3D/Object/Model/ModelStructs.h>
#include "Graphics/Resource/ResourceManager.h"

class DirectXManager;
class SrvManager;

/// <summary>
/// スキニングの「入力側」GPUリソース。メッシュ1つにつき1個で、全インスタンスが共有する。
///
/// 入力頂点もインフルエンスもアニメーションで変化しないので、生成時に1回書けば以後触らない。
/// （以前は毎フレーム入力頂点を丸ごとUploadバッファへコピーし直していた）
/// </summary>
class SkinningResource
{
public:
	~SkinningResource();

	void Initialize(
		const SkinnedMeshData& meshData,
		const std::map<std::string, JointWeightData>& skinClusterData,
		const SkeletonData& bindSkeleton,
		DirectXManager* dxManager,
		SrvManager* srvManager);

	uint32_t GetVertexCount() const { return vertexCount_; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetInputVertexSRV() const { return inputVertexSRV_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetInfluenceSRV() const { return influenceSRV_; }

private:
	void CreateInfluence(const SkinnedMeshData& meshData,
		const std::map<std::string, JointWeightData>& skinClusterData,
		const SkeletonData& bindSkeleton);
	void CreateInputVertex(const SkinnedMeshData& meshData);

private:
	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	uint32_t vertexCount_ = 0;

	BufferHandle influenceHandle_ = kInvalidBufferHandle;
	BufferHandle inputVertexHandle_ = kInvalidBufferHandle;

	uint32_t influenceSrvIndex_ = UINT32_MAX;
	uint32_t inputVertexSrvIndex_ = UINT32_MAX;

	D3D12_GPU_DESCRIPTOR_HANDLE influenceSRV_{};
	D3D12_GPU_DESCRIPTOR_HANDLE inputVertexSRV_{};
};
