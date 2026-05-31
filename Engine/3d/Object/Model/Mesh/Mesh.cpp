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
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &skinCluster_->GetOutputVBV());
	}

	directXManager_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	directXManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::BindForGBuffer()
{
	if (skinnedMeshData_.skinClusterData.size() == 0) {
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	} else {
		directXManager_->GetCommandList()->IASetVertexBuffers(0, 1, &skinCluster_->GetOutputVBV());
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
