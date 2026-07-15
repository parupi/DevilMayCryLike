#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <mutex>
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOCommon.h"

class PSOManager {
public:
	void Initialize(DirectXManager* dxManager);
	void Finalize();

public:
	// スプライト
	ID3D12RootSignature* GetSpriteSignature() { return spriteSignature_.Get(); }
	ID3D12PipelineState* GetSpritePSO(BlendMode blendMode);

	ID3D12RootSignature* GetParticleSignature() { return particleSignature_.Get(); }
	ID3D12PipelineState* GetParticlePSO(BlendMode blendMode);

	ID3D12RootSignature* GetObjectSignature() { return objectSignature_.Get(); }
	ID3D12PipelineState* GetObjectPSO(BlendMode blendMode);

	ID3D12RootSignature* GetAnimationSignature() { return animationSignature_.Get(); }
	ID3D12PipelineState* GetAnimationPSO();

	ID3D12RootSignature* GetOffScreenSignature() { return offScreenSignature_.Get(); }
	ID3D12PipelineState* GetOffScreenPSO(OffScreenEffectType effectType);

	ID3D12RootSignature* GetPrimitiveSignature() { return primitiveSignature_.Get(); }
	ID3D12PipelineState* GetPrimitivePSO();

	ID3D12RootSignature* GetSkyboxSignature() { return skyboxSignature_.Get(); }
	ID3D12PipelineState* GetSkyboxPSO();

	ID3D12RootSignature* GetSkinningSignature() { return skinningSignature_.Get(); }
	ID3D12PipelineState* GetSkinningPSO();

	// DeferredRendering
	ID3D12RootSignature* GetDeferredSignature() { return deferredSignature_.Get(); }
	ID3D12PipelineState* GetDeferredPSO();

	// LightingPath
	ID3D12RootSignature* GetLightingPathSignature() { return lightingPathSignature_.Get(); }
	ID3D12PipelineState* GetLightingPathPSO();

	// FinalComposite
	ID3D12RootSignature* GetFinalCompositeRootSignature() { return finalCompositeRootSignature_.Get(); }
	ID3D12PipelineState* GetFinalCompositePSO(bool isPostCopy = false);

	// Composite
	ID3D12RootSignature* GetCompositeRootSignature() { return compositeRootSignature_.Get(); }
	ID3D12PipelineState* GetCompositePSO();

	// CascadedShadowMap
	ID3D12RootSignature* GetCSMRootSignature() { return csmRootSignature_.Get(); }
	ID3D12PipelineState* GetCSMPSO();

	// Trail
	ID3D12RootSignature* GetTrailSignature() { return trailSignature_.Get(); }
	ID3D12PipelineState* GetTrailPSO();

private:
	void CreateSpriteSignature();
	void CreateSpritePSO(BlendMode blendMode);
	void CreateParticleSignature();
	void CreateParticlePSO(BlendMode blendMode);
	void CreateObjectSignature();
	void CreateObjectPSO(BlendMode blendMode);
	void CreateAnimationSignature();
	void CreateAnimationPSO();
	void CreateOffScreenSignature();
	void CreateOffScreenPSO(OffScreenEffectType effectType);
	void CreatePrimitiveSignature();
	void CreatePrimitivePSO();
	void CreateSkyboxSignature();
	void CreateSkyboxPSO();
	void CreateSkinningSignature();
	void CreateSkinningPSO();
	void CreateDeferredSignature();
	void CreateDeferredPSO();
	void CreateLightingPathSignature();
	void CreateLightingPathPSO();
	void CreateFinalCompositeRootSignature();
	void CreateFinalCompositePSO(bool isPostCopy);
	void CreateCompositeRootSignature();
	void CreateCompositePSO();
	void CreateCSMRootSignature();
	void CreateCSMPSO();
	void CreateTrailSignature();
	void CreateTrailPSO();

private:
	DirectXManager* dxManager_ = nullptr;

private:
	// スプライト
	Microsoft::WRL::ComPtr<ID3D12RootSignature> spriteSignature_;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> spriteGraphicsPipelineState_;
	// パーティクル
	Microsoft::WRL::ComPtr<ID3D12RootSignature> particleSignature_;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> particleGraphicsPipelineState_;
	// オブジェクト
	Microsoft::WRL::ComPtr<ID3D12RootSignature> objectSignature_;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> objectGraphicsPipelineState_;
	// アニメーション
	Microsoft::WRL::ComPtr<ID3D12RootSignature> animationSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> animationGraphicsPipelineState_;
	// オフスクリーン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> offScreenSignature_;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> offScreenGraphicsPipelineState_;
	// プリミティブ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> primitiveSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> primitiveGraphicsPipelineState_;
	// スカイボックス
	Microsoft::WRL::ComPtr<ID3D12RootSignature> skyboxSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skyboxGraphicsPipelineState_;
	// ComputeSkinning
	Microsoft::WRL::ComPtr<ID3D12RootSignature> skinningSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skinningComputePipelineState_;
	// DeferredRendering
	Microsoft::WRL::ComPtr<ID3D12RootSignature> deferredSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> deferredPipelineState_;
	// LightingPath
	Microsoft::WRL::ComPtr<ID3D12RootSignature> lightingPathSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> lightingPathPipelineState_;
	// FinalComposite
	Microsoft::WRL::ComPtr<ID3D12RootSignature> finalCompositeRootSignature_;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 2> finalCompositePSO_;
	// Forward/Deferred合成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> compositeRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> compositePSO_;
	// CascadedShadowMap
	Microsoft::WRL::ComPtr<ID3D12RootSignature> csmRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> csmPSO_;
	// Trail
	Microsoft::WRL::ComPtr<ID3D12RootSignature> trailSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> trailPSO_;
};
