#pragma once
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;

class SkinningPipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature);
};
