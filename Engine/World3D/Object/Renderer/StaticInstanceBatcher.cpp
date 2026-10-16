#include "StaticInstanceBatcher.h"
#include "ModelRenderer.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "World3D/Object/Model/Model.h"
#include "World3D/WorldTransform.h"
#include "Utility/Logger.h"
#include "Utility/ScopeProfiler.h"

void StaticInstanceBatcher::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;

	auto* resourceManager = dxManager_->GetResourceManager();
	instanceBuffer_ = resourceManager->CreateUploadBuffer(sizeof(InstanceTransform) * kCapacity, L"StaticInstanceBatcher");
	mapped_ = reinterpret_cast<InstanceTransform*>(resourceManager->Map(instanceBuffer_));
}

void StaticInstanceBatcher::BeginFrame()
{
	cursor_ = 0;

	lastGBufferInstances_ = gbufferInstances_;
	lastGBufferDraws_ = gbufferDraws_;
	lastShadowInstances_ = shadowInstances_;
	lastShadowDraws_ = shadowDraws_;
	gbufferInstances_ = 0;
	gbufferDraws_ = 0;
	shadowInstances_ = 0;
	shadowDraws_ = 0;
}

StaticInstanceBatcher::Group& StaticInstanceBatcher::FindGroup(std::vector<Group>& groups, Model* model, bool environmentSurface)
{
	// グループはモデルの種類ぶん（いまは数個）しか無いので線形で探す
	for (Group& group : groups) {
		if (group.model == model && group.environmentSurface == environmentSurface) {
			return group;
		}
	}
	Group& group = groups.emplace_back();
	group.model = model;
	group.environmentSurface = environmentSurface;
	return group;
}

void StaticInstanceBatcher::SubmitGBuffer(ModelRenderer* renderer, Model* model, bool environmentSurface)
{
	FindGroup(gbufferGroups_, model, environmentSurface).items.push_back(renderer);
}

void StaticInstanceBatcher::SubmitShadow(ModelRenderer* renderer, Model* model)
{
	// 影は深度だけなので、環境オブジェクトかどうかで分ける必要がない
	FindGroup(shadowGroups_, model, false).items.push_back(renderer);
}

D3D12_GPU_VIRTUAL_ADDRESS StaticInstanceBatcher::WriteInstances(const Group& group)
{
	const uint32_t count = static_cast<uint32_t>(group.items.size());
	if (cursor_ + count > kCapacity) {
		if (!overflowLogged_) {
			Logger::Log("StaticInstanceBatcher: インスタンスのバッファが足りません。kCapacity を増やしてください\n");
			overflowLogged_ = true;
		}
		return 0;
	}

	for (uint32_t i = 0; i < count; ++i) {
		WorldTransform* transform = group.items[i]->GetWorldTransform();
		InstanceTransform& dst = mapped_[cursor_ + i];
		dst.world = transform->GetMatWorld();
		dst.worldInverseTranspose = transform->GetWorldInverseTranspose();
	}

	const D3D12_GPU_VIRTUAL_ADDRESS address =
		dxManager_->GetResourceManager()->GetGPUVirtualAddress(instanceBuffer_) + sizeof(InstanceTransform) * cursor_;
	cursor_ += count;
	return address;
}

void StaticInstanceBatcher::FlushGBuffer(const Matrix4x4& viewProj)
{
	bool any = false;
	for (const Group& group : gbufferGroups_) {
		any |= !group.items.empty();
	}
	if (!any) return;

	PROF_SCOPE("Obj:インスタンス描画(GBuffer)");
	auto* cmd = dxManager_->GetCommandList();
	cmd->SetPipelineState(psoManager_->GetDeferredInstancedPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetDeferredInstancedSignature());
	cmd->SetGraphicsRoot32BitConstants(1, 16, &viewProj, 0);

	for (Group& group : gbufferGroups_) {
		if (group.items.empty()) continue;
		const D3D12_GPU_VIRTUAL_ADDRESS address = WriteInstances(group);
		if (address != 0) {
			// b2: ディゾルブ無効・発光なし（まとめるのはそういうものだけ）。z は環境オブジェクトの印。
			// 値は GBufferPath::Begin の既定値と同じ
			const float constants[12] = {
				-1.0f, 0.05f, group.environmentSurface ? 1.0f : 0.0f, 0.0f,
				1.0f, 0.3f, 0.0f, 8.0f,
				0.0f, 0.0f, 0.0f, 0.0f,
			};
			cmd->SetGraphicsRoot32BitConstants(4, 12, constants, 0);
			cmd->SetGraphicsRootShaderResourceView(5, address);
			group.model->DrawGBufferInstanced(static_cast<uint32_t>(group.items.size()));

			gbufferInstances_ += static_cast<uint32_t>(group.items.size());
			gbufferDraws_ += 1;
		}
		group.items.clear();
	}

	// このあと通常の描画が続いても困らないよう、GBufferPath::Begin と同じ状態へ戻す
	cmd->SetPipelineState(psoManager_->GetDeferredPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetDeferredSignature());
	const float dissolveDefaults[12] = { -1.0f, 0.05f, 0.0f, 0.0f, 1.0f, 0.3f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	cmd->SetGraphicsRoot32BitConstants(4, 12, dissolveDefaults, 0);
}

void StaticInstanceBatcher::FlushShadow(const Matrix4x4& lightViewProj)
{
	bool any = false;
	for (const Group& group : shadowGroups_) {
		any |= !group.items.empty();
	}
	if (!any) return;

	PROF_SCOPE("Obj:インスタンス描画(影)");
	auto* cmd = dxManager_->GetCommandList();
	cmd->SetPipelineState(psoManager_->GetCSMInstancedPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetCSMInstancedRootSignature());
	cmd->SetGraphicsRoot32BitConstants(1, 16, &lightViewProj, 0);

	for (Group& group : shadowGroups_) {
		if (group.items.empty()) continue;
		// カスケードごとに別の範囲へ書く（前のカスケードの行列を GPU が読む前に上書きしないため）
		const D3D12_GPU_VIRTUAL_ADDRESS address = WriteInstances(group);
		if (address != 0) {
			cmd->SetGraphicsRootShaderResourceView(0, address);
			group.model->DrawShadowInstanced(static_cast<uint32_t>(group.items.size()));

			shadowInstances_ += static_cast<uint32_t>(group.items.size());
			shadowDraws_ += 1;
		}
		group.items.clear();
	}

	// 次のカスケードは ShadowPass が通常の PSO のまま b1 を結び直すので、ここで戻しておく
	cmd->SetPipelineState(psoManager_->GetCSMPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetCSMRootSignature());
}
