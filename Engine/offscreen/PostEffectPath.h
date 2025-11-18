#pragma once

#include <d3d12.h>
#include <stdint.h>

class DirectXManager;
class PSOManager;
class OffScreenManager;
class BaseOffScreen;

class PostEffectPath
{
public:
	PostEffectPath(BaseOffScreen* effect);
	~PostEffectPath() = default;

	// 初期化
	//void Initialize(std::unique_ptr<BaseOffScreen> effect);
	// 描画実行
	void Execute();

	bool IsActive() const;

	void SetInputSRV(D3D12_GPU_DESCRIPTOR_HANDLE srv);
	void SetOutput(ID3D12Resource* target, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
	void SetViewport(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);

	D3D12_GPU_DESCRIPTOR_HANDLE GetOutputSRV() const { return outputSrv_; }
private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	OffScreenManager* offscreen_ = nullptr;

	BaseOffScreen* effect_ = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv_{};
	D3D12_GPU_DESCRIPTOR_HANDLE outputSrv_{};
	uint32_t outputSrvIndex_ = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};
	ID3D12Resource* outputResource_ = nullptr;

};

