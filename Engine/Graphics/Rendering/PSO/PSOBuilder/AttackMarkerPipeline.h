#pragma once
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;

/// <summary>
/// 敵の攻撃予兆マーカー（地面に描く赤い図形）用のパイプライン。
/// テクスチャを持たず手続き的に描くので、ルートシグネチャは viewProj の b0 だけ。
/// </summary>
class AttackMarkerPipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature);
};
