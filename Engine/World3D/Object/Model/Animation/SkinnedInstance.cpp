#include "SkinnedInstance.h"
#include "SkinningResource.h"
#include "World3D/Object/Model/SkinnedModel.h"
#include "World3D/Object/Model/Mesh/Mesh.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"
#include <algorithm>

SkinnedInstance::~SkinnedInstance()
{
	if (!dxManager_) return;

	auto* resourceManager = dxManager_->GetResourceManager();

	mappedPalette_ = nullptr;
	if (paletteHandle_ != kInvalidBufferHandle) resourceManager->ReleaseBuffer(paletteHandle_);
	if (srvManager_ && paletteSrvIndex_ != UINT32_MAX) srvManager_->Free(paletteSrvIndex_);

	for (auto& output : meshOutputs_) {
		if (output.outputHandle != kInvalidBufferHandle) resourceManager->ReleaseBuffer(output.outputHandle);
		if (output.infoHandle != kInvalidBufferHandle) resourceManager->ReleaseBuffer(output.infoHandle);
		if (srvManager_ && output.uavIndex != UINT32_MAX) srvManager_->Free(output.uavIndex);
	}
	meshOutputs_.clear();
}

void SkinnedInstance::Initialize(SkinnedModel* asset)
{
	asset_ = asset;
	dxManager_ = asset->GetDxManager();
	srvManager_ = asset->GetSrvManager();

	// バインドポーズのスケルトンをコピーして、このインスタンスのポーズにする
	skeleton_ = asset->GetBindSkeleton();
	jointCount_ = static_cast<uint32_t>(skeleton_.GetJointCount());

	player_ = std::make_unique<AnimationPlayer>();
	player_->Initialize(asset->GetClipSet(), &skeleton_);

	CreatePalette(jointCount_);

	for (size_t i = 0; i < asset->GetMeshCount(); ++i) {
		CreateMeshOutput(asset->GetSkinningResource(i));
	}

	// 生成直後に一度ポーズと行列を作っておく。
	// Update前に描かれてもパレットが単位行列のまま潰れて見えることがない
	player_->Update(0.0f);
	Update(0.0f);
}

void SkinnedInstance::CreatePalette(uint32_t jointCount)
{
	auto* resourceManager = dxManager_->GetResourceManager();

	const size_t size = sizeof(WellForGPU) * (std::max)(jointCount, 1u);
	paletteHandle_ = resourceManager->CreateUploadBuffer(size, L"Skin:Palette");
	mappedPalette_ = reinterpret_cast<WellForGPU*>(resourceManager->Map(paletteHandle_));

	for (uint32_t i = 0; i < jointCount; ++i) {
		mappedPalette_[i].skeletonSpaceMatrix = MakeIdentity4x4();
		mappedPalette_[i].skeletonSpaceInverseTransposeMatrix = MakeIdentity4x4();
	}

	paletteSrvIndex_ = srvManager_->Allocate();
	paletteSRV_ = srvManager_->GetGPUDescriptorHandle(paletteSrvIndex_);
	srvManager_->CreateSRVforStructuredBuffer(
		paletteSrvIndex_, resourceManager->GetResource(paletteHandle_),
		jointCount, sizeof(WellForGPU));
}

void SkinnedInstance::CreateMeshOutput(const SkinningResource* shared)
{
	auto* resourceManager = dxManager_->GetResourceManager();

	MeshOutput output;
	output.shared = shared;
	output.vertexCount = shared->GetVertexCount();

	const size_t size = sizeof(VertexData) * output.vertexCount;
	output.outputHandle = resourceManager->CreateDefaultBuffer(
		size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"Skin:OutputVertex");

	auto* resource = resourceManager->GetResource(output.outputHandle);

	output.uavIndex = srvManager_->Allocate();
	output.uav = srvManager_->GetGPUDescriptorHandle(output.uavIndex);
	srvManager_->CreateUAVforStructuredBuffer(output.uavIndex, resource, output.vertexCount, sizeof(VertexData));

	output.vbv.BufferLocation = resource->GetGPUVirtualAddress();
	output.vbv.SizeInBytes = static_cast<UINT>(size);
	output.vbv.StrideInBytes = sizeof(VertexData);

	// 頂点数を渡すだけの定数バッファ
	output.infoHandle = resourceManager->CreateUploadBuffer(sizeof(SkinningInformation), L"Skin:Info");
	auto* info = reinterpret_cast<SkinningInformation*>(resourceManager->Map(output.infoHandle));
	info->vertexCount = output.vertexCount;

	meshOutputs_.push_back(output);
}

void SkinnedInstance::Update(float deltaTime)
{
	if (!player_ || !mappedPalette_) return;

	// ポーズを進める（内部で Skeleton::Update まで済む）
	player_->Update(deltaTime);

	// 行列パレットへ書き込む。
	// パレットは Upload ヒープ（write-combine）なので書くだけにし、
	// 元になる逆バインドポーズは必ずアセット側のCPU配列から読む
	const std::vector<Matrix4x4>& inverseBindPose = asset_->GetInverseBindPoseMatrices();
	const SkeletonData& skeletonData = skeleton_.GetSkeletonData();

	const size_t count = (std::min)(inverseBindPose.size(), skeletonData.joints.size());
	for (size_t i = 0; i < count; ++i) {
		const Matrix4x4 mat = inverseBindPose[i] * skeletonData.joints[i].skeletonSpaceMatrix;
		mappedPalette_[i].skeletonSpaceMatrix = mat;
		mappedPalette_[i].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(mat));
	}
}

void SkinnedInstance::DispatchSkinning()
{
	auto* cmd = dxManager_->GetCommandList();

	for (auto& output : meshOutputs_) {
		if (output.vertexCount == 0) continue;

		auto* resource = dxManager_->GetResourceManager()->GetResource(output.outputHandle);

		cmd->SetComputeRootDescriptorTable(0, paletteSRV_);
		cmd->SetComputeRootDescriptorTable(1, output.shared->GetInputVertexSRV());
		cmd->SetComputeRootDescriptorTable(2, output.shared->GetInfluenceSRV());
		cmd->SetComputeRootDescriptorTable(3, output.uav);
		cmd->SetComputeRootConstantBufferView(
			4, dxManager_->GetResourceManager()->GetResource(output.infoHandle)->GetGPUVirtualAddress());

		const uint32_t numGroups = (output.vertexCount + 1023) / 1024;
		cmd->Dispatch(numGroups, 1, 1);

		// 頂点バッファとして読めるようにする。
		// バッファは ExecuteCommandLists 完了時に COMMON へ decay し、次フレームの
		// Dispatch で UAV へ暗黙昇格するので、戻り側のバリアは要らない
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		cmd->ResourceBarrier(1, &barrier);
	}
}

const D3D12_VERTEX_BUFFER_VIEW& SkinnedInstance::GetOutputVBV(size_t meshIndex) const
{
	static const D3D12_VERTEX_BUFFER_VIEW kEmpty{};
	if (meshIndex >= meshOutputs_.size()) return kEmpty;
	return meshOutputs_[meshIndex].vbv;
}
