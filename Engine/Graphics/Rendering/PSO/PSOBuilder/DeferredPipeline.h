#pragma once
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;

class DeferredPipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature);

	// 静的モデルのインスタンシング用（StaticInstanceBatcher）。
	// 並びは通常版と同じで、b1 をカメラの VP（ルート定数16個）に替え、t2（インスタンスの行列）を足したもの
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateInstancedRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateInstancedPSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature);

private:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignatureImpl(DirectXManager* dxManager, bool instanced);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSOImpl(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature, const wchar_t* vertexShaderPath);
};
