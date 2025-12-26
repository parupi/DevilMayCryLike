#include "ForwardRenderTargetArray.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"

void ForwardRenderTargetArray::Initialize(DirectXManager* dxManager, uint32_t layerCount)
{
    layerCount_ = layerCount;

    auto* resourceFactory = dxManager->GetResourceFactory();

    // ==============================
    // Color Texture2DArray
    // ==============================
    GpuResourceFactory::TextureDesc colorDesc{};
    colorDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    colorDesc.usage = GpuResourceFactory::Usage::RenderTarget;
    colorDesc.arraySize = static_cast<uint16_t>(layerCount_);

    colorResource_ = resourceFactory->CreateTexture2D(colorDesc);
    colorResource_->SetName(L"ForwardColorArray");

    colorSrvIndex_ = dxManager->GetSrvManager()->Allocate();
    dxManager->GetSrvManager()->CreateSRVforTexture2DArray(colorSrvIndex_, colorResource_.Get(), colorDesc.format, 1, layerCount_);

    dxManager->GetCommandContext()->TransitionResource(colorResource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // ==============================
    // Depth Texture2DArray
    // ==============================
    GpuResourceFactory::TextureDesc depthDesc{};
    depthDesc.format = DXGI_FORMAT_R24G8_TYPELESS;
    depthDesc.usage = GpuResourceFactory::Usage::DepthStencil;
    depthDesc.arraySize = static_cast<uint16_t>(layerCount_);

    depthResource_ = resourceFactory->CreateTexture2D(depthDesc);
    depthResource_->SetName(L"ForwardDepthArray");

    depthSrvIndex_ = dxManager->GetSrvManager()->Allocate();
    dxManager->GetSrvManager()->CreateSRVforTexture2DArray(depthSrvIndex_, depthResource_.Get(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1, layerCount_);

    dxManager->GetCommandContext()->TransitionResource(depthResource_.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
