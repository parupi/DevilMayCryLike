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

void Mesh::Draw()
{
	// VertexBufferViewを設定
	directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	directXManager_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	directXManager_->GetCommandList()->DrawIndexedInstanced(UINT(meshData_.indices.size()), 1, 0, 0, 0);
}

void Mesh::Bind()
{
	// 頂点バッファをセット
	directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// インデックスバッファをセット
	directXManager_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	// トポロジ（プリミティブの種類）もここで設定しておくと安心
	directXManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
