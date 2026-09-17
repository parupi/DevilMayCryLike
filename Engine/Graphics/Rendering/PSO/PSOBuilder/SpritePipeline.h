#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Graphics/Rendering/PSO/PSOCommon.h"

class DirectXManager;

class SpritePipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	/// <param name="toBackBuffer">
	/// true にするとバックバッファ(R8G8B8A8_UNORM_SRGB・深度なし)向けのPSOを作る。
	/// ポストエフェクトの後にUIを直接描くための版。
	/// false はシーン用RT(R16G16B16A16_FLOAT・深度あり)向け。
	/// </param>
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature, BlendMode blendMode, bool toBackBuffer = false);
};
