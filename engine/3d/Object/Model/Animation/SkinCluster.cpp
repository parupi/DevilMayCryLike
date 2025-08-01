#include "SkinCluster.h"
#include "3d/Object/Model/SkinnedModel.h"
#include "3d/Object/Model/Animation/Skeleton.h"
#include "base/DirectXManager.h"
#include "base/SrvManager.h"

SkinCluster::~SkinCluster()
{
	// マッピング解除
	if (skinCluster_.paletteResource) {
		skinCluster_.paletteResource->Unmap(0, nullptr);
		skinCluster_.mappedPalette = {};
	}

	if (skinCluster_.influenceResource) {
		skinCluster_.influenceResource->Unmap(0, nullptr);
		skinCluster_.mappedInfluence = {};
	}

	if (skinCluster_.inputVertexResource) {
		skinCluster_.inputVertexResource->Unmap(0, nullptr);
		skinCluster_.mappedInputVertex = {};
	}

	if (skinCluster_.skinningInfoResource) {
		skinCluster_.skinningInfoResource->Unmap(0, nullptr);
		skinCluster_.skinningInfoData = nullptr;
	}

	// outputVertex は UAV 専用のため、マップしていない（mappedOutputVertex はビュー情報）
	skinCluster_.mappedOutputVertex = {};

	// ComPtr によるリソースの自動解放に任せる
	skinCluster_.paletteResource.Reset();
	skinCluster_.influenceResource.Reset();
	skinCluster_.inputVertexResource.Reset();
	skinCluster_.outputVertexResource.Reset();
	skinCluster_.skinningInfoResource.Reset();

	// ポインタやハンドル類をクリア
	skinCluster_.paletteSrvHandle = {};
	skinCluster_.influenceSrvHandle = {};
	skinCluster_.inputVertexSrvHandle = {};
	skinCluster_.outputVertexSrvHandle = {};

	// その他
	skinCluster_.inverseBindPoseMatrices.clear();
}


void SkinCluster::Initialize(const SkeletonData& skeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData, DirectXManager* dxManager, SrvManager* srvManager)
{
	dxManager_ = dxManager;
	srvManager_ = srvManager;

	vertexCount_ = static_cast<uint32_t>(meshData.vertices.size());

	CreateSkinCluster(skeleton, meshData, skinClusterData);
}

void SkinCluster::CreateSkinCluster(const SkeletonData& skeleton, const SkinnedMeshData& meshData, const std::map<std::string, JointWeightData>& skinClusterData)
{
	CreatePaletteResource(skeleton);
	CreateInfluenceResource(meshData);
	CreateInputVertexResource();
	CreateOutputVertexResource();
	CreateSkinningInfoResource();

	// InverseBindPoseMatrixを格納する場所を探し、単位行列で埋める
	skinCluster_.inverseBindPoseMatrices.resize(vertexCount_);
	std::generate(skinCluster_.inverseBindPoseMatrices.begin(), skinCluster_.inverseBindPoseMatrices.end(), MakeIdentity4x4);

	for (const auto& jointWeight : skinClusterData) { // ModelのSkinClusterの情報を解析
		auto it = skeleton.jointMap.find(jointWeight.first); // JointWeight.firstはJoint名
		if (it == skeleton.jointMap.end()) { // 見つからなかったら次に回す
			continue;
		}
		// (*it).secondにはjointのindexが入っている
		skinCluster_.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster_.mappedInfluence[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
				if (currentInfluence.weights[index] == 0.0f) {
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}
}

void SkinCluster::UpdateSkinning()
{
	auto* commandList = dxManager_->GetCommandList();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = skinCluster_.outputVertexResource.Get();
	commandList->ResourceBarrier(1, &barrier);

	commandList->SetComputeRootDescriptorTable(0, skinCluster_.paletteSrvHandle.second);

	commandList->SetComputeRootDescriptorTable(1, skinCluster_.inputVertexSrvHandle.second);

	commandList->SetComputeRootDescriptorTable(2, skinCluster_.influenceSrvHandle.second);

	commandList->SetComputeRootDescriptorTable(3, skinCluster_.outputVertexSrvHandle.second);

	commandList->SetComputeRootConstantBufferView(4, skinCluster_.skinningInfoResource->GetGPUVirtualAddress());

	// Dispatch実行
	uint32_t numGroups = (static_cast<uint32_t>(vertexCount_) + 1023) / 1024;
	commandList->Dispatch(numGroups, 1, 1);

	// 4. UAV -> VBV バリア
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = skinCluster_.outputVertexResource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	commandList->ResourceBarrier(1, &barrier);
}

void SkinCluster::UpdateInputVertex(const SkinnedMeshData& meshData)
{
	for (size_t i = 0; i < meshData.vertices.size(); ++i) {
		skinCluster_.mappedInputVertex[i] = meshData.vertices[i];
	}
}

void SkinCluster::UpdateSkinCluster(const SkeletonData& skeleton)
{
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix = skinCluster_.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

void SkinCluster::CreatePaletteResource(const SkeletonData& skeleton)
{
	// palette用のResource確保
	dxManager_->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size(), skinCluster_.paletteResource);
	WellForGPU* mappedPalette = nullptr;
	skinCluster_.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster_.mappedPalette = { mappedPalette, vertexCount_ };
	skinCluster_.paletteIndex = srvManager_->Allocate();
	skinCluster_.paletteSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinCluster_.paletteIndex);
	skinCluster_.paletteSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinCluster_.paletteIndex);

	srvManager_->CreateSRVforStructuredBuffer(skinCluster_.paletteIndex, skinCluster_.paletteResource.Get(), UINT(skeleton.joints.size()), sizeof(WellForGPU));
}

void SkinCluster::CreateInfluenceResource(const SkinnedMeshData& meshData)
{
	// inputVertex用のResourceを確保
	dxManager_->CreateBufferResource(sizeof(VertexInfluence) * meshData.vertices.size(), skinCluster_.influenceResource);
	VertexInfluence* mappedVertex = nullptr;
	skinCluster_.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertex));
	skinCluster_.mappedInfluence = { mappedVertex, meshData.vertices.size() };
	skinCluster_.influenceIndex = srvManager_->Allocate();
	skinCluster_.influenceSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinCluster_.influenceIndex);
	skinCluster_.influenceSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinCluster_.influenceIndex);

	srvManager_->CreateSRVforStructuredBuffer(skinCluster_.influenceIndex, skinCluster_.influenceResource.Get(), UINT(meshData.vertices.size()), sizeof(VertexInfluence));
}

void SkinCluster::CreateInputVertexResource()
{
	// inputVertex用のResource確保
	dxManager_->CreateBufferResource(sizeof(VertexData) * vertexCount_, skinCluster_.inputVertexResource);
	VertexData* mappedPalette = nullptr;
	skinCluster_.inputVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster_.mappedInputVertex = { mappedPalette, vertexCount_ };
	skinCluster_.inputVertexIndex = srvManager_->Allocate();
	skinCluster_.inputVertexSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinCluster_.inputVertexIndex);
	skinCluster_.inputVertexSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinCluster_.inputVertexIndex);

	srvManager_->CreateSRVforStructuredBuffer(skinCluster_.inputVertexIndex, skinCluster_.inputVertexResource.Get(), UINT(vertexCount_), sizeof(VertexData));
}

void SkinCluster::CreateOutputVertexResource()
{
	// outputVertex用のResource確保
	dxManager_->CreateBufferResource(sizeof(VertexData) * vertexCount_, skinCluster_.outputVertexResource, true);

	skinCluster_.outputVertexIndex = srvManager_->Allocate();
	skinCluster_.outputVertexSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinCluster_.outputVertexIndex);
	skinCluster_.outputVertexSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinCluster_.outputVertexIndex);

	srvManager_->CreateUAVforStructuredBuffer(skinCluster_.outputVertexIndex, skinCluster_.outputVertexResource.Get(), UINT(vertexCount_), sizeof(VertexData));

	skinCluster_.mappedOutputVertex.BufferLocation = skinCluster_.outputVertexResource->GetGPUVirtualAddress();
	skinCluster_.mappedOutputVertex.SizeInBytes = UINT(sizeof(VertexData) * UINT(vertexCount_));
	skinCluster_.mappedOutputVertex.StrideInBytes = sizeof(VertexData);
}

void SkinCluster::CreateSkinningInfoResource()
{
	// SkinningInformation用のResourceを確保
	dxManager_->CreateBufferResource(sizeof(SkinningInformation), skinCluster_.skinningInfoResource);
	skinCluster_.skinningInfoData = nullptr;
	skinCluster_.skinningInfoResource->Map(0, nullptr, reinterpret_cast<void**>(&skinCluster_.skinningInfoData));
	skinCluster_.skinningInfoData->vertexCount = static_cast<uint32_t>(vertexCount_);
}
