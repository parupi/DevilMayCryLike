#pragma once
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>
class GraphicsDevice
{
public:
	GraphicsDevice() = default;
	~GraphicsDevice();
	// デバイスの生成
	bool Initialize();

	ID3D12Device* GetDevice() const { return device_.Get(); }
	IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }
private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
};

