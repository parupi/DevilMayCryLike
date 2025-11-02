#pragma once
#include <d3d12.h>
#include <base/DirectXManager.h>
class GBufferPass
{
public:
	GBufferPass() = default;
	~GBufferPass() = default;
	void Initialize(DirectXManager* dxManager);
	void Begin();
	void End();

	const D3D12_CPU_DESCRIPTOR_HANDLE* GetRTVs() const { return rtvHandles_; }

private:
	DirectXManager* dxManager_ = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_;
};

