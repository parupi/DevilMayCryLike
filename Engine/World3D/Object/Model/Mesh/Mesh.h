#pragma once
#include <World3D/Object/Model/ModelStructs.h>
#include <World3D/Object/Model/Animation/SkinCluster.h>
#include <memory>
#include "Graphics/Resource/ResourceManager.h"

class SrvManager;
class DirectXManager;

class Mesh {
public:

	~Mesh();
	// 初期化処理
	void Initialize(DirectXManager* directXManager, SrvManager* srvManager, const MeshData& meshData);

	void Initialize(DirectXManager* directXManager, SrvManager* srvManager, const SkinnedMeshData& meshData);
	// 描画処理
	void Update();

	void Bind();
	// GBufferへのバインド
	void BindForGBuffer();

	void CreateSkinCluster(const SkeletonData& skeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData);

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return vertexBufferView_; }

	const MeshData& GetMeshData() { return meshData_; }
	const SkinnedMeshData& GetSkinnedMeshData() { return skinnedMeshData_; }

	SkinCluster* GetSkinCluster() { return skinCluster_.get(); }
private:

	// 頂点データの生成
	void CreateVertexResource();
	// Indexデータの生成
	void CreateIndexResource();

private:
	DirectXManager* directXManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	BufferHandle vertexHandle_ = kInvalidBufferHandle;
	BufferHandle indexHandle_ = kInvalidBufferHandle;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	MeshData meshData_;
	SkinnedMeshData skinnedMeshData_;

	std::unique_ptr<SkinCluster> skinCluster_; // 追加
};