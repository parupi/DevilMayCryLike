#include "PSOManager.h"
#include "PSOBuilder/SpritePipeline.h"
#include "PSOBuilder/ParticlePipeline.h"
#include "PSOBuilder/ObjectPipeline.h"
#include "PSOBuilder/AnimationPipeline.h"
#include "PSOBuilder/OffScreenPipeline.h"
#include "PSOBuilder/PrimitivePipeline.h"
#include "PSOBuilder/SkyboxPipeline.h"
#include "PSOBuilder/SkinningPipeline.h"
#include "PSOBuilder/DeferredPipeline.h"
#include "PSOBuilder/LightingPathPipeline.h"
#include "PSOBuilder/FinalCompositePipeline.h"
#include "PSOBuilder/CompositePipeline.h"
#include "PSOBuilder/CSMPipeline.h"
#include "PSOBuilder/TrailPipeline.h"
#include <cassert>

void PSOManager::Initialize(DirectXManager* dxManager) {
	dxManager_ = dxManager;
}

void PSOManager::Finalize() {
	spriteSignature_.Reset();
	for (auto& pso : spriteGraphicsPipelineState_) { pso.Reset(); }

	particleSignature_.Reset();
	for (auto& pso : particleGraphicsPipelineState_) { pso.Reset(); }

	objectSignature_.Reset();
	for (auto& pso : objectGraphicsPipelineState_) { pso.Reset(); }

	animationSignature_.Reset();
	animationGraphicsPipelineState_.Reset();

	offScreenSignature_.Reset();
	for (auto& pso : offScreenGraphicsPipelineState_) { pso.Reset(); }

	primitiveSignature_.Reset();
	primitiveGraphicsPipelineState_.Reset();

	skyboxSignature_.Reset();
	skyboxGraphicsPipelineState_.Reset();

	skinningSignature_.Reset();
	skinningComputePipelineState_.Reset();

	deferredSignature_.Reset();
	deferredPipelineState_.Reset();

	lightingPathSignature_.Reset();
	lightingPathPipelineState_.Reset();

	finalCompositeRootSignature_.Reset();
	for (auto& pso : finalCompositePSO_) { pso.Reset(); }

	compositeRootSignature_.Reset();
	compositePSO_.Reset();

	csmRootSignature_.Reset();
	csmPSO_.Reset();

	trailSignature_.Reset();
	trailPSO_.Reset();

	dxManager_ = nullptr;
}

// ---------------------------------------------------------------------------
// Sprite
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetSpritePSO(BlendMode blendMode) {
	if (!spriteGraphicsPipelineState_[static_cast<UINT>(blendMode)]) {
		CreateSpritePSO(blendMode);
	}
	return spriteGraphicsPipelineState_[static_cast<UINT>(blendMode)].Get();
}

void PSOManager::CreateSpriteSignature() {
	if (!spriteSignature_) {
		spriteSignature_ = SpritePipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateSpritePSO(BlendMode blendMode) {
	CreateSpriteSignature();
	spriteGraphicsPipelineState_[static_cast<UINT>(blendMode)] =
		SpritePipeline::CreatePSO(dxManager_, spriteSignature_.Get(), blendMode);
}

// ---------------------------------------------------------------------------
// Particle
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetParticlePSO(BlendMode blendMode) {
	if (!particleGraphicsPipelineState_[static_cast<UINT>(blendMode)]) {
		CreateParticlePSO(blendMode);
	}
	return particleGraphicsPipelineState_[static_cast<UINT>(blendMode)].Get();
}

void PSOManager::CreateParticleSignature() {
	if (!particleSignature_) {
		particleSignature_ = ParticlePipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateParticlePSO(BlendMode blendMode) {
	CreateParticleSignature();
	particleGraphicsPipelineState_[static_cast<UINT>(blendMode)] =
		ParticlePipeline::CreatePSO(dxManager_, particleSignature_.Get(), blendMode);
}

// ---------------------------------------------------------------------------
// Object
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetObjectPSO(BlendMode blendMode) {
	if (!objectGraphicsPipelineState_[static_cast<UINT>(blendMode)].Get()) {
		CreateObjectPSO(blendMode);
	}
	return objectGraphicsPipelineState_[static_cast<UINT>(blendMode)].Get();
}

void PSOManager::CreateObjectSignature() {
	if (!objectSignature_) {
		objectSignature_ = ObjectPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateObjectPSO(BlendMode blendMode) {
	CreateObjectSignature();
	objectGraphicsPipelineState_[static_cast<UINT>(blendMode)] =
		ObjectPipeline::CreatePSO(dxManager_, objectSignature_.Get(), blendMode);
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetAnimationPSO() {
	if (!animationGraphicsPipelineState_) {
		CreateAnimationPSO();
	}
	return animationGraphicsPipelineState_.Get();
}

void PSOManager::CreateAnimationSignature() {
	if (!animationSignature_) {
		animationSignature_ = AnimationPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateAnimationPSO() {
	CreateAnimationSignature();
	animationGraphicsPipelineState_ =
		AnimationPipeline::CreatePSO(dxManager_, animationSignature_.Get());
}

// ---------------------------------------------------------------------------
// OffScreen
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetOffScreenPSO(OffScreenEffectType effectType) {
	if (!offScreenGraphicsPipelineState_[static_cast<UINT>(effectType)].Get()) {
		CreateOffScreenPSO(effectType);
	}
	return offScreenGraphicsPipelineState_[static_cast<UINT>(effectType)].Get();
}

void PSOManager::CreateOffScreenSignature() {
	if (!offScreenSignature_) {
		offScreenSignature_ = OffScreenPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateOffScreenPSO(OffScreenEffectType effectType) {
	CreateOffScreenSignature();
	offScreenGraphicsPipelineState_[static_cast<UINT>(effectType)] =
		OffScreenPipeline::CreatePSO(dxManager_, offScreenSignature_.Get(), effectType);
}

// ---------------------------------------------------------------------------
// Primitive
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetPrimitivePSO() {
	if (!primitiveGraphicsPipelineState_) {
		CreatePrimitivePSO();
	}
	return primitiveGraphicsPipelineState_.Get();
}

void PSOManager::CreatePrimitiveSignature() {
	if (!primitiveSignature_) {
		primitiveSignature_ = PrimitivePipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreatePrimitivePSO() {
	CreatePrimitiveSignature();
	primitiveGraphicsPipelineState_ =
		PrimitivePipeline::CreatePSO(dxManager_, primitiveSignature_.Get());
}

// ---------------------------------------------------------------------------
// Skybox
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetSkyboxPSO() {
	if (!skyboxGraphicsPipelineState_) {
		CreateSkyboxPSO();
	}
	return skyboxGraphicsPipelineState_.Get();
}

void PSOManager::CreateSkyboxSignature() {
	if (!skyboxSignature_) {
		skyboxSignature_ = SkyboxPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateSkyboxPSO() {
	CreateSkyboxSignature();
	skyboxGraphicsPipelineState_ =
		SkyboxPipeline::CreatePSO(dxManager_, skyboxSignature_.Get());
}

// ---------------------------------------------------------------------------
// Skinning (Compute)
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetSkinningPSO() {
	if (!skinningComputePipelineState_) {
		CreateSkinningPSO();
	}
	return skinningComputePipelineState_.Get();
}

void PSOManager::CreateSkinningSignature() {
	if (!skinningSignature_) {
		skinningSignature_ = SkinningPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateSkinningPSO() {
	CreateSkinningSignature();
	skinningComputePipelineState_ =
		SkinningPipeline::CreatePSO(dxManager_, skinningSignature_.Get());
}

// ---------------------------------------------------------------------------
// Deferred
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetDeferredPSO() {
	if (!deferredPipelineState_) {
		CreateDeferredPSO();
	}
	return deferredPipelineState_.Get();
}

void PSOManager::CreateDeferredSignature() {
	if (!deferredSignature_) {
		deferredSignature_ = DeferredPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateDeferredPSO() {
	CreateDeferredSignature();
	deferredPipelineState_ =
		DeferredPipeline::CreatePSO(dxManager_, deferredSignature_.Get());
}

// ---------------------------------------------------------------------------
// LightingPath
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetLightingPathPSO() {
	if (!lightingPathPipelineState_) {
		CreateLightingPathPSO();
	}
	return lightingPathPipelineState_.Get();
}

void PSOManager::CreateLightingPathSignature() {
	if (!lightingPathSignature_) {
		lightingPathSignature_ = LightingPathPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateLightingPathPSO() {
	CreateLightingPathSignature();
	lightingPathPipelineState_ =
		LightingPathPipeline::CreatePSO(dxManager_, lightingPathSignature_.Get());
}

// ---------------------------------------------------------------------------
// FinalComposite
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetFinalCompositePSO(bool isPostCopy) {
	const uint32_t idx = isPostCopy ? 0u : 1u;
	if (!finalCompositePSO_[idx]) {
		CreateFinalCompositePSO(isPostCopy);
	}
	return finalCompositePSO_[idx].Get();
}

void PSOManager::CreateFinalCompositeRootSignature() {
	if (!finalCompositeRootSignature_) {
		finalCompositeRootSignature_ = FinalCompositePipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateFinalCompositePSO(bool isPostCopy) {
	CreateFinalCompositeRootSignature();
	const uint32_t idx = isPostCopy ? 0u : 1u;
	finalCompositePSO_[idx] =
		FinalCompositePipeline::CreatePSO(dxManager_, finalCompositeRootSignature_.Get(), isPostCopy);
}

// ---------------------------------------------------------------------------
// Composite
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetCompositePSO() {
	if (!compositePSO_) {
		CreateCompositePSO();
	}
	return compositePSO_.Get();
}

void PSOManager::CreateCompositeRootSignature() {
	if (!compositeRootSignature_) {
		compositeRootSignature_ = CompositePipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateCompositePSO() {
	CreateCompositeRootSignature();
	compositePSO_ = CompositePipeline::CreatePSO(dxManager_, compositeRootSignature_.Get());
}

// ---------------------------------------------------------------------------
// CascadedShadowMap
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetCSMPSO() {
	if (!csmPSO_) {
		CreateCSMPSO();
	}
	return csmPSO_.Get();
}

void PSOManager::CreateCSMRootSignature() {
	if (!csmRootSignature_) {
		csmRootSignature_ = CSMPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateCSMPSO() {
	CreateCSMRootSignature();
	csmPSO_ = CSMPipeline::CreatePSO(dxManager_, csmRootSignature_.Get());
}

// ---------------------------------------------------------------------------
// Trail
// ---------------------------------------------------------------------------
ID3D12PipelineState* PSOManager::GetTrailPSO() {
	if (!trailPSO_) {
		CreateTrailPSO();
	}
	return trailPSO_.Get();
}

void PSOManager::CreateTrailSignature() {
	if (!trailSignature_) {
		trailSignature_ = TrailPipeline::CreateRootSignature(dxManager_);
	}
}

void PSOManager::CreateTrailPSO() {
	CreateTrailSignature();
	trailPSO_ = TrailPipeline::CreatePSO(dxManager_, trailSignature_.Get());
}
