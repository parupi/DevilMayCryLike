#pragma once
#include <3d/Object/Model/ModelStructs.h>
#include <3d/Object/Model/Animation/SkinCluster.h>
#include <memory>

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

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	MeshData meshData_;
	SkinnedMeshData skinnedMeshData_;

	std::unique_ptr<SkinCluster> skinCluster_; // 追加
};