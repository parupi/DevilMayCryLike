#pragma once
#include <d3d12.h>
#include <cstdint>
#include <wrl.h>

class DirectXManager;

class ForwardRenderTargetArray
{
public:
	void Initialize(DirectXManager* dxManager, uint32_t layerCount);

    // ---- Resource（RTV / DSV 作成用）----
    ID3D12Resource* GetColorResource() const { return colorResource_.Get(); }
    ID3D12Resource* GetDepthResource() const { return depthResource_.Get(); }

    uint32_t GetColorSrv() const { return colorSrvIndex_; }
    uint32_t GetDepthSrv() const { return depthSrvIndex_; }

    uint32_t GetLayerCount() const { return layerCount_; }
private:
    uint32_t layerCount_ = 0;

    // Texture2DArray
    Microsoft::WRL::ComPtr<ID3D12Resource> colorResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_;

    // SRV index
    uint32_t colorSrvIndex_ = 0;
    uint32_t depthSrvIndex_ = 0;
};

