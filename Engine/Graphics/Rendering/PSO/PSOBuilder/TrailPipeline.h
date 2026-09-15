#pragma once
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;

class TrailPipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	/// <param name="additive">true で加算合成（光る帯）、false で半透明の合成（明るい床の上でも色が残る帯）</param>
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature, bool additive = true);
};
