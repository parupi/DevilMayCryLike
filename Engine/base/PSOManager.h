#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <mutex>
#include "DirectXManager.h"

enum class BlendMode {
	kNone,
	kNormal,
	kAdd,
	kSubtract,
	kMultiply,
	kScreen,
};

enum class OffScreenEffectType {
	kNone,
	kGray,
	kVignette,
	kSmooth,
	kGauss,
	kOutLine,
	kDepth
};

class PSOManager {
public:
	// 初期化
	void Initialize(DirectXManager* dxManager);
	// 終了処理
	void Finalize();

public:// アクセッサ
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
private:
	// スプライト
	void CreateSpriteSignature();
	void CreateSpritePSO(BlendMode blendMode);
	// パーティクル
	void CreateParticleSignature();
	void CreateParticlePSO(BlendMode blendMode);
	// オブジェクト
	void CreateObjectSignature();
	void CreateObjectPSO(BlendMode blendMode);
	// アニメーション
	void CreateAnimationSignature();
	void CreateAnimationPSO();
	// オフスクリーン
	void CreateOffScreenSignature();
	void CreateOffScreenPSO(OffScreenEffectType effectType);
	// プリミティブ
	void CreatePrimitiveSignature();
	void CreatePrimitivePSO();
	// スカイボックス
	void CreateSkyboxSignature();
	void CreateSkyboxPSO();
	// ComputeSkinning
	void CreateSkinningSignature();
	void CreateSkinningPSO();
	// DeferredRendering
	void CreateDeferredSignature();
	void CreateDeferredPSO();
	// LightingPath
	void CreateLightingPathSignature();
	void CreateLightingPathPSO();
private:
	DirectXManager* dxManager_ = nullptr;

private: // データ格納用変数
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
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 7> offScreenGraphicsPipelineState_;
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
};