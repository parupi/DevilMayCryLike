#include "GpuResourceFactory.h"
#include <cassert>
#include <DirectXTex/d3dx12.h>

using Microsoft::WRL::ComPtr;

//------------------------------------------
// 2D テクスチャ生成
//------------------------------------------
ComPtr<ID3D12Resource> GpuResourceFactory::CreateTexture2D(const TextureDesc& desc)
{
    D3D12_RESOURCE_DESC texture{};
    texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width = desc.width;
    texture.Height = desc.height;
    texture.DepthOrArraySize = 1;
    texture.MipLevels = 1;
    texture.Format = desc.format;
    texture.SampleDesc.Count = 1;
    texture.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture.Flags = GetResourceFlags(desc.usage);

    // Heap 設定
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // 初期ステート
    D3D12_RESOURCE_STATES initState = GetInitialState(desc.usage);

    // ClearValue（使う用途に応じて）
    D3D12_CLEAR_VALUE clearValue{};
    bool useClearValue = false;

    if (desc.usage == Usage::RenderTarget) {
        clearValue.Format = desc.format;
        memcpy(clearValue.Color, desc.clearColor, sizeof(float) * 4);
        useClearValue = true;
    } else if (desc.usage == Usage::DepthStencil) {
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = desc.clearDepth;
        clearValue.DepthStencil.Stencil = 0;
        useClearValue = true;
    }

    ComPtr<ID3D12Resource> resource;
    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texture,
        initState,
        useClearValue ? &clearValue : nullptr,
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

#ifdef _DEBUG
    resource->SetName(L"CreatedTexture2D");
#endif

    return resource;
}

ComPtr<ID3D12Resource> GpuResourceFactory::CreateTexture2D(const DirectX::TexMetadata& meta, D3D12_RESOURCE_STATES initialState)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(meta.dimension);
    desc.Alignment = 0;
    desc.Width = meta.width;
    desc.Height = static_cast<UINT>(meta.height);
    desc.DepthOrArraySize = static_cast<UINT16>(meta.arraySize);
    desc.MipLevels = static_cast<UINT16>(meta.mipLevels);
    desc.Format = meta.format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ComPtr<ID3D12Resource> tex;

    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(tex.GetAddressOf())
    );

    assert(SUCCEEDED(hr));

    return tex;
}

//------------------------------------------
// 初期ステート
//------------------------------------------
D3D12_RESOURCE_STATES GpuResourceFactory::GetInitialState(Usage usage)
{
    switch (usage) {
    case Usage::RenderTarget:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;

    case Usage::DepthStencil:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;

    case Usage::UAV:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    case Usage::ShaderResource:
        // Shader 全ステージで使用可能な SRV の初期ステート
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    default:
        return D3D12_RESOURCE_STATE_COMMON;
    }
}

//------------------------------------------
// リソースフラグ
//------------------------------------------
D3D12_RESOURCE_FLAGS GpuResourceFactory::GetResourceFlags(Usage usage)
{
    switch (usage) {
    case Usage::RenderTarget:
        return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    case Usage::DepthStencil:
        return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    case Usage::UAV:
        return D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    case Usage::ShaderResource:
    default:
        return D3D12_RESOURCE_FLAG_NONE;
    }
}