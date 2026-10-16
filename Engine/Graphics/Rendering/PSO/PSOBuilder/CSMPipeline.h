#pragma once
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;

class CSMPipeline {
public:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature);

	// 静的モデルのインスタンシング用（StaticInstanceBatcher）。
	// t0: インスタンスの行列（StructuredBuffer） / b1: ライトの VP（ルート定数16個）
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateInstancedRootSignature(DirectXManager* dxManager);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateInstancedPSO(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature);

private:
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> SerializeAndCreate(DirectXManager* dxManager, const D3D12_ROOT_SIGNATURE_DESC& desc);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSOImpl(
		DirectXManager* dxManager, ID3D12RootSignature* rootSignature, const wchar_t* vertexShaderPath);
};
