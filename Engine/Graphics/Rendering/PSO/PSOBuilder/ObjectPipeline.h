#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Graphics/Rendering/PSO/PSOCommon.h"

class DirectXManager;

class ObjectPipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature, BlendMode blendMode);
};
