#pragma once
#include <memory>
#include <d3d12.h>

class DirectXManager;
class GBufferManager;
class PSOManager;

class GBufferPath
{
public:
	void Initialize(DirectXManager* dxManager, GBufferManager* gBuffer, PSOManager* psoManager);

	void Begin();

	void Draw();

	void End();

	void SetInputTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) { inputSrv_ = handle; }
private:
	DirectXManager* dxManager_ = nullptr;
	GBufferManager* gBuffer_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv_{};
};

