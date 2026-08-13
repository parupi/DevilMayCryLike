#include "SkinningResource.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"

SkinningResource::~SkinningResource()
{
	if (!dxManager_) return;

	auto* resourceManager = dxManager_->GetResourceManager();
	if (influenceHandle_ != kInvalidBufferHandle) resourceManager->ReleaseBuffer(influenceHandle_);
	if (inputVertexHandle_ != kInvalidBufferHandle) resourceManager->ReleaseBuffer(inputVertexHandle_);

	if (srvManager_) {
		if (influenceSrvIndex_ != UINT32_MAX) srvManager_->Free(influenceSrvIndex_);
		if (inputVertexSrvIndex_ != UINT32_MAX) srvManager_->Free(inputVertexSrvIndex_);
	}
}

void SkinningResource::Initialize(
	const SkinnedMeshData& meshData,
	const std::map<std::string, JointWeightData>& skinClusterData,
	const SkeletonData& bindSkeleton,
	DirectXManager* dxManager,
	SrvManager* srvManager)
{
	dxManager_ = dxManager;
	srvManager_ = srvManager;
	vertexCount_ = static_cast<uint32_t>(meshData.vertices.size());

	CreateInfluence(meshData, skinClusterData, bindSkeleton);
	CreateInputVertex(meshData);
}

void SkinningResource::CreateInfluence(
	const SkinnedMeshData& meshData,
	const std::map<std::string, JointWeightData>& skinClusterData,
	const SkeletonData& bindSkeleton)
{
	auto* resourceManager = dxManager_->GetResourceManager();

	const size_t size = sizeof(VertexInfluence) * meshData.vertices.size();
	influenceHandle_ = resourceManager->CreateUploadBuffer(size, L"Skin:Influence");

	auto* mapped = reinterpret_cast<VertexInfluence*>(resourceManager->Map(influenceHandle_));

	for (const auto& [jointName, weightData] : skinClusterData) {
		auto found = bindSkeleton.jointMap.find(jointName);
		if (found == bindSkeleton.jointMap.end()) continue;

		const int32_t jointIndex = found->second;
		for (const auto& vw : weightData.vertexWeights) {
			if (vw.vertexIndex >= meshData.vertices.size()) continue;

			auto& influence = mapped[vw.vertexIndex];
			for (uint32_t i = 0; i < kNumMaxInfluence; i++) {
				if (influence.weights[i] == 0.0f) {
					influence.weights[i] = vw.weight;
					influence.jointIndices[i] = jointIndex;
					break;
				}
			}
			// kNumMaxInfluence を超えるウェイトは黙って捨てられる。
			// 4本を超えるリグを使うならここで一番小さいものと入れ替える処理が要る
		}
	}

	influenceSrvIndex_ = srvManager_->Allocate();
	influenceSRV_ = srvManager_->GetGPUDescriptorHandle(influenceSrvIndex_);
	srvManager_->CreateSRVforStructuredBuffer(
		influenceSrvIndex_, resourceManager->GetResource(influenceHandle_),
		vertexCount_, sizeof(VertexInfluence));
}

void SkinningResource::CreateInputVertex(const SkinnedMeshData& meshData)
{
	auto* resourceManager = dxManager_->GetResourceManager();

	const size_t size = sizeof(VertexData) * meshData.vertices.size();
	inputVertexHandle_ = resourceManager->CreateUploadBuffer(size, L"Skin:InputVertex");

	// 入力頂点はアニメーションで変わらないのでここで1回書くだけ
	void* mapped = resourceManager->Map(inputVertexHandle_);
	std::memcpy(mapped, meshData.vertices.data(), size);

	inputVertexSrvIndex_ = srvManager_->Allocate();
	inputVertexSRV_ = srvManager_->GetGPUDescriptorHandle(inputVertexSrvIndex_);
	srvManager_->CreateSRVforStructuredBuffer(
		inputVertexSrvIndex_, resourceManager->GetResource(inputVertexHandle_),
		vertexCount_, sizeof(VertexData));
}
