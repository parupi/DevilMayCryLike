#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"
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

public:// アクセッサ
	// スプライト
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetSpriteSignature() { return spriteSignature_; }
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetSpritePSO(BlendMode blendMode);
	// パーティクル
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetParticleSignature() { return particleSignature_; }
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetParticlePSO(BlendMode blendMode);
	// オブジェクト
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetObjectSignature() { return objectSignature_; }
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetObjectPSO();
	// アニメーション
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetAnimationSignature() { return animationSignature_; }
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetAnimationPSO();
	// オフスクリーン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetOffScreenSignature() { return offScreenSignature_; }
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetOffScreenPSO(OffScreenEffectType effectType);

private:
	// スプライト
	void CreateSpriteSignature();
	void CreateSpritePSO(BlendMode blendMode);
	// パーティクル
	void CreateParticleSignature();
	void CreateParticlePSO(BlendMode blendMode);
	// オブジェクト
	void CreateObjectSignature();
	void CreateObjectPSO();
	// アニメーション
	void CreateAnimationSignature();
	void CreateAnimationPSO();
	// オフスクリーン
	void CreateOffScreenSignature();
	void CreateOffScreenPSO(OffScreenEffectType effectType);
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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> objectGraphicsPipelineState_;
	// アニメーション
	Microsoft::WRL::ComPtr<ID3D12RootSignature> animationSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> animationGraphicsPipelineState_;
	// オフスクリーン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> offScreenSignature_;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 7> offScreenGraphicsPipelineState_;
};