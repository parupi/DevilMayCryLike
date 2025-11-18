#include "Mesh.h"
#include "base/DirectXManager.h"
#include "base/SrvManager.h"

Mesh::~Mesh() {
	if (vertexResource_) {
		vertexResource_->Unmap(0, nullptr);
		vertexResource_.Reset();  // ← ここでリソースを解放
		vertexData_ = nullptr;
	}
	if (indexResource_) {
		indexResource_->Unmap(0, nullptr);
		indexResource_.Reset();   // ← ここでリソースを解放
		indexData_ = nullptr;
	}
}

void Mesh::Initialize(DirectXManager* directXManager, SrvManager* srvManager, const MeshData& meshData)
{
	directXManager_ = directXManager;
	srvManager_ = srvManager;

	meshData_ = meshData;

	CreateVertexResource();

	CreateIndexResource();
}

void Mesh::Initialize(DirectXManager* directXManager, SrvManager* srvManager, const SkinnedMeshData& meshData)
{
	directXManager_ = directXManager;
	srvManager_ = srvManager;

	skinnedMeshData_ = meshData;
	// skinnedMeshから普通のメッシュデータに抽出
	meshData_.indices = skinnedMeshData_.indices;
	meshData_.materialIndex = skinnedMeshData_.materialIndex;
	meshData_.name = skinnedMeshData_.name;
	meshData_.vertices = skinnedMeshData_.vertices;

	CreateVertexResource();

	CreateIndexResource();
}

void Mesh::Update()
{
	skinCluster_->UpdateSkinning();
}

void Mesh::Bind()
{
	// VertexBufferViewを設定
	// スキニングしていなければ普通の
	if (skinnedMeshData_.skinClusterData.size() == 0) {
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	} else {
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &skinCluster_->GetSkinCluster().mappedOutputVertex);
	}

	directXManager_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	directXManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::BindForGBuffer()
{
	if (skinnedMeshData_.skinClusterData.size() == 0) {
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	} else {
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &skinCluster_->GetSkinCluster().mappedOutputVertex);
	}

	directXManager_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	directXManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::CreateSkinCluster(const SkeletonData& skeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData)
{
	skinCluster_ = std::make_unique<SkinCluster>();
	skinCluster_->Initialize(skeleton, meshData, skinClusterData, directXManager_, srvManager_);
}

void Mesh::CreateVertexResource()
{
	// 頂点リソースを作る
	directXManager_->CreateBufferResource(sizeof(VertexData) * meshData_.vertices.size(), vertexResource_);
	// 頂点バッファビューを作成する
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();	// リソースの先頭アドレスから使う
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * meshData_.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData); // 1頂点当たりのサイズ
	// 頂点リソースにデータを書き込む
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_)); // 書き込むためのアドレスを取得
	std::memcpy(vertexData_, meshData_.vertices.data(), sizeof(VertexData) * meshData_.vertices.size());
	
	// ログ出力
	Logger::LogBufferCreation("Mesh:Vertex", vertexResource_.Get(), meshData_.vertices.size());
}

void Mesh::CreateIndexResource()
{
	directXManager_->CreateBufferResource(sizeof(uint32_t) * meshData_.indices.size(), indexResource_);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * meshData_.indices.size());
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	std::memcpy(indexData_, meshData_.indices.data(), sizeof(uint32_t) * meshData_.indices.size());

	// ログ出力
	Logger::LogBufferCreation("Mesh:Index", indexResource_.Get(), meshData_.indices.size());
}
