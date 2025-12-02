#include "GBufferManager.h"
#include <base/DirectXManager.h>

void GBufferManager::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;

	rtvIncrement_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 各種ヒープの生成
	rtvHeap_ = dxManager_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, (UINT)GBufferType::Count, false);
	dsvHeap_ = dxManager_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	CreateResources(WindowManager::kClientWidth, WindowManager::kClientHeight);
	CreateRTVs();
	CreateSRVs();
	CreateDSV();

    TransitionAllToReadable();
}

void GBufferManager::Finalize()
{
    for (auto& resource : gBufferResources_) {
        resource.Reset();
    }

    rtvHeap_.Reset();
    dsvHeap_.Reset();

    dxManager_ = nullptr;
}

void GBufferManager::TransitionAllToReadable()
{
    auto* commandContext = dxManager_->GetCommandContext();

    // Depth は SRV を作っていないなら除外する
    for (size_t i = 0; i < (size_t)GBufferType::Count; ++i) {
        if (!gBufferResources_[i]) continue;

        // Depth が D24_UNORM_S8_UINT で SRVを持っていない場合はスキップする
        if (gBufferResources_[i]->GetDesc().Format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
            continue;
        }

        commandContext->TransitionResource(
            gBufferResources_[i].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }
}

void GBufferManager::TransitionAllToRT()
{
    auto* commandContext = dxManager_->GetCommandContext();

    for (size_t i = 0; i < (size_t)GBufferType::Count; ++i) {
        if (!gBufferResources_[i]) continue;

        DXGI_FORMAT format = gBufferResources_[i]->GetDesc().Format;

        // 深度フォーマットは RenderTarget として扱えないのでスキップ
        if (format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
            continue;
        }

        commandContext->TransitionResource(
            gBufferResources_[i].Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
    }
}

void GBufferManager::CreateResources(UINT width, UINT height)
{
    GpuResourceFactory* resourceFactory = dxManager_->GetResourceFactory();

    GpuResourceFactory::TextureDesc desc{};
    desc.width = width;
    desc.height = height;

    //===========================
    // Albedo
    //===========================
    desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GpuResourceFactory::Usage::RenderTarget;
    desc.clearColor[0] = 0.6f;
    desc.clearColor[1] = 0.5f;
    desc.clearColor[2] = 0.1f;
    desc.clearColor[3] = 1.0f;
    gBufferResources_[(size_t)GBufferType::Albedo] =
        resourceFactory->CreateTexture2D(desc);

    //===========================
    // Normal
    //===========================
    desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.clearColor[0] = 0.0f;
    desc.clearColor[1] = 0.5f;
    desc.clearColor[2] = 0.0f;
    desc.clearColor[3] = 1.0f;
    gBufferResources_[(size_t)GBufferType::Normal] =
        resourceFactory->CreateTexture2D(desc);

    //===========================
    // WorldPos
    //===========================
    desc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.clearColor[0] = 0.0f;
    desc.clearColor[1] = 0.0f;
    desc.clearColor[2] = 0.0f;
    desc.clearColor[3] = 1.0f;
    gBufferResources_[(size_t)GBufferType::WorldPos] =
        resourceFactory->CreateTexture2D(desc);

    //===========================
    // Material
    //===========================
    desc.format = DXGI_FORMAT_R8G8_UNORM;
    desc.clearColor[0] = 0.0f;
    desc.clearColor[1] = 0.0f;
    desc.clearColor[2] = 0.0f;
    desc.clearColor[3] = 0.0f; // Material は不要なので0
    gBufferResources_[(size_t)GBufferType::Material] =
        resourceFactory->CreateTexture2D(desc);

    //===========================
    // Depth
    //===========================
    {
        GpuResourceFactory::TextureDesc depthDesc{};
        depthDesc.width = width;
        depthDesc.height = height;
        depthDesc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.usage = GpuResourceFactory::Usage::DepthStencil;
        depthDesc.clearDepth = 1.0f;

        gBufferResources_[(size_t)GBufferType::Depth] =
            resourceFactory->CreateTexture2D(depthDesc);
    }
}


void GBufferManager::CreateRTVs()
{
    auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    dxManager_->GetDevice()->CreateRenderTargetView(gBufferResources_[(size_t)GBufferType::Albedo].Get(), nullptr, handle);
    handle.ptr += rtvIncrement_;

    dxManager_->GetDevice()->CreateRenderTargetView(gBufferResources_[(size_t)GBufferType::Normal].Get(), nullptr, handle);
    handle.ptr += rtvIncrement_;
    dxManager_->GetDevice()->CreateRenderTargetView(gBufferResources_[(size_t)GBufferType::WorldPos].Get(), nullptr, handle);
}

void GBufferManager::CreateSRVs()
{
    auto device = dxManager_->GetDevice();

    for (size_t i = 0; i < (size_t)GBufferType::Count; i++) {
        // Depthの場合はスキップ
        if (gBufferResources_[i]->GetDesc().Format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
            continue;
        }

        UINT idx = dxManager_->GetSrvManager()->Allocate();

        auto cpu = dxManager_->GetSrvManager()->GetCPUDescriptorHandle(idx);

        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Format = gBufferResources_[i]->GetDesc().Format;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = 1;

        dxManager_->GetDevice()->CreateShaderResourceView(gBufferResources_[i].Get(), &desc, cpu);

        srvIndices_[i] = idx; // GPUHandleを後で取得するため保存
    }
}


void GBufferManager::CreateDSV()
{
    auto handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    dxManager_->GetDevice()->CreateDepthStencilView(gBufferResources_[(size_t)GBufferType::Depth].Get(), nullptr, handle);
}

ID3D12Resource* GBufferManager::GetResource(GBufferType type) const
{
    return gBufferResources_[(size_t)type].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GBufferManager::GetRTVHandle(GBufferType type) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    h.ptr += (size_t)type * rtvIncrement_;
    return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE GBufferManager::GetDSVHandle() const
{
    return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE GBufferManager::GetSRVHandle(GBufferType type) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE h = dxManager_->GetSrvManager()->GetGPUDescriptorHandle(srvIndices_[(size_t)type]);
    return h;
}