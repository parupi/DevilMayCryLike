#pragma once
#include <3d/Object/Model/ModelStructs.h>

class SrvManager;
class DirectXManager;

class Mesh {
public:

	~Mesh();
	// 初期化処理
	void Initialize(DirectXManager* directXManager, SrvManager* srvManager, const MeshData& meshData);
	// 描画処理
	void Draw();

	void Bind();

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

public:
	MeshData& GetMeshData() { return meshData_; }

};