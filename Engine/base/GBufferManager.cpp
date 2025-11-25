#include "GBufferManager.h"
#include <base/DirectXManager.h>

void GBufferManager::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;

	rtvIncrement_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//srvIncrement_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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
        if (format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
            format == DXGI_FORMAT_D32_FLOAT ||
            format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
        {
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
    // Albedo
    D3D12_CLEAR_VALUE albedoClear{};
    albedoClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    albedoClear.Color[0] = 0.0f;
    albedoClear.Color[1] = 0.0f;
    albedoClear.Color[2] = 0.0f;
    albedoClear.Color[3] = 1.0f;
    gBufferResources_[(size_t)GBufferType::Albedo] = dxManager_->CreateGBufferResource(width, height, albedoClear);

    // Normal
    D3D12_CLEAR_VALUE normalClear{};
    normalClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    normalClear.Color[0] = 0.5f; // デコード前に0.5固定
    normalClear.Color[1] = 0.5f;
    normalClear.Color[2] = 1.0f; // +Z向き
    normalClear.Color[3] = 1.0f;
    gBufferResources_[(size_t)GBufferType::Normal] = dxManager_->CreateGBufferResource(width, height, normalClear);

    // WorldPos
    D3D12_CLEAR_VALUE worldPosClear{};
    worldPosClear.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    worldPosClear.Color[0] = 0.0f;
    worldPosClear.Color[1] = 0.0f;
    worldPosClear.Color[2] = 0.0f;
    worldPosClear.Color[3] = 1.0f; // alpha は 1.0
    gBufferResources_[(size_t)GBufferType::WorldPos] = dxManager_->CreateGBufferResource(width, height, worldPosClear);

    // Material
    D3D12_CLEAR_VALUE materialClear{};
    materialClear.Format = DXGI_FORMAT_R8G8_UNORM;
    materialClear.Color[0] = 0.0f;
    materialClear.Color[1] = 0.0f;
    gBufferResources_[(size_t)GBufferType::Material] = dxManager_->CreateGBufferResource(width, height, materialClear);

    // Depth
    gBufferResources_[(size_t)GBufferType::Depth] = dxManager_->CreateDepthStencilTextureResource(width, height, DXGI_FORMAT_D24_UNORM_S8_UINT);
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