#include "Mesh.h"
#include "Graphics/Device/DirectXManager.h"

Mesh::~Mesh()
{
	if (directXManager_) {
		auto* resourceManager = directXManager_->GetResourceManager();
		if (vertexHandle_ != kInvalidBufferHandle) {
			resourceManager->ReleaseBuffer(vertexHandle_);
			vertexHandle_ = kInvalidBufferHandle;
		}
		if (indexHandle_ != kInvalidBufferHandle) {
			resourceManager->ReleaseBuffer(indexHandle_);
			indexHandle_ = kInvalidBufferHandle;
		}
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

void Mesh::Bind(const D3D12_VERTEX_BUFFER_VIEW* vbvOverride)
{
	auto* commandList = directXManager_->GetCommandList();

	// スキンモデルは変形後の頂点（インスタンスが持つ出力バッファ）を使う
	commandList->IASetVertexBuffers(0, 1, vbvOverride ? vbvOverride : &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::CreateSkinningResource(const SkeletonData& bindSkeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData)
{
	skinningResource_ = std::make_unique<SkinningResource>();
	skinningResource_->Initialize(meshData, skinClusterData, bindSkeleton, directXManager_, srvManager_);
}

void Mesh::CreateVertexResource()
{
	auto* resourceManager = directXManager_->GetResourceManager();
	const size_t vertexSize = sizeof(VertexData) * meshData_.vertices.size();

	vertexHandle_ = resourceManager->CreateUploadBuffer(vertexSize, L"Mesh:Vertex");

	void* ptr = resourceManager->Map(vertexHandle_);
	assert(ptr);
	std::memcpy(ptr, meshData_.vertices.data(), vertexSize);

	ID3D12Resource* resource = resourceManager->GetResource(vertexHandle_);
	vertexBufferView_.BufferLocation = resource->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexSize);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	Logger::LogBufferCreation("Mesh:Vertex", resource, meshData_.vertices.size());
}

void Mesh::CreateIndexResource()
{
	auto* resourceManager = directXManager_->GetResourceManager();
	const size_t indexSize = sizeof(uint32_t) * meshData_.indices.size();

	indexHandle_ = resourceManager->CreateUploadBuffer(indexSize, L"Mesh:Index");

	void* ptr = resourceManager->Map(indexHandle_);
	assert(ptr);
	std::memcpy(ptr, meshData_.indices.data(), indexSize);

	ID3D12Resource* resource = resourceManager->GetResource(indexHandle_);
	indexBufferView_.BufferLocation = resource->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(indexSize);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	Logger::LogBufferCreation("Mesh:Index", resource, meshData_.indices.size());
}
