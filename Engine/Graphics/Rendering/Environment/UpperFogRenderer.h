#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Graphics/Resource/ResourceManager.h"

class DirectXManager;

/// <summary>
/// 上部の霧の層を描く。
/// </summary>
class UpperFogRenderer {
public:
	static UpperFogRenderer& GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();

	/// <summary>GBuffer のワールド座標の SRV。RenderPipeline が渡す</summary>
	void SetSceneWorldPositionSrv(D3D12_GPU_DESCRIPTOR_HANDLE srv) { sceneWorldPosSrv_ = srv; }

	/// <summary>フォワードパスの中（不透明物の後）で呼ぶ。RT と深度は設定済みの前提</summary>
	void Draw();

private:
	UpperFogRenderer() = default;
	UpperFogRenderer(const UpperFogRenderer&) = delete;
	UpperFogRenderer& operator=(const UpperFogRenderer&) = delete;

	void CreateRootSignature();
	void CreatePSO();

	// HLSL の FogLayerCB と並びを合わせること（FogLayer.hlsli）。1層ぶんを256バイト境界に置く
	struct FogLayerCB {
		float viewProj[16];
		float center[3];
		float halfSize;
		float color[3];   // リニア
		float opacity;
		float cameraPos[3];
		float time;
		float noiseOffset[2]; // 流れた距離（速さ×時間）
		float noiseScale;
		float softDistance;
		float edgeFade;
		float layerSeed;  // 層ごとにノイズの模様をずらす
		float grazingFade; // 1 なら視線が水平に近いほど薄くする
		float _pad;
	};
	static constexpr UINT kSlotSize = 256;
	static constexpr int kMaxLayers = 4;

	DirectXManager* dxManager_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
	BufferHandle bufferHandle_ = kInvalidBufferHandle;
	unsigned char* mapped_ = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE sceneWorldPosSrv_{};

	// 霧が流れた時間。ヒットストップ中は止まるよう VFX の時間で進める
	float time_ = 0.0f;
};
