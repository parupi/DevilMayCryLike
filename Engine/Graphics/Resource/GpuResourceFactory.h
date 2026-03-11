#pragma once
#include "base/WindowManager.h"
#include <d3d12.h>
#include <wrl.h>
#include <DirectXTex/DirectXTex.h>
class GpuResourceFactory
{
public:
	enum class Usage {
		ShaderResource,
		RenderTarget,
		DepthStencil,
		UAV,
	};

	struct TextureDesc {
		uint32_t width = WindowManager::kClientWidth;
		uint32_t height = WindowManager::kClientHeight;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		Usage usage = Usage::ShaderResource;

		float clearColor[4] = { 0,0,0,1 };
		float clearDepth = 1.0f;
	};

	GpuResourceFactory(ID3D12Device* device)
		: device_(device) {
	}

	// 2Dテクスチャ生成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture2D(const TextureDesc& desc);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture2D(const DirectX::TexMetadata& meta, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST);
private:
	ID3D12Device* device_;

	// 初期ステートの設定
	D3D12_RESOURCE_STATES GetInitialState(Usage usage);
	// リソースフラグの取得
	D3D12_RESOURCE_FLAGS GetResourceFlags(Usage usage);
};

