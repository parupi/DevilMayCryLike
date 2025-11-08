#include "GBufferManager.h"
#include <base/DirectXManager.h>

GBufferManager* GBufferManager::instance = nullptr;
std::once_flag GBufferManager::initInstanceFlag;

GBufferManager* GBufferManager::GetInstance()
{
    std::call_once(initInstanceFlag, []() {
        instance = new GBufferManager();
        });
    return instance;
}

void GBufferManager::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;

	rtvIncrement_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	srvIncrement_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 各種ヒープの生成
	rtvHeap_ = dxManager_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, (UINT)GBufferType::Count, false);
	srvHeap_ = dxManager_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (UINT)GBufferType::Count, true);
	dsvHeap_ = dxManager_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	CreateResources(WindowManager::kClientWidth, WindowManager::kClientHeight);
	CreateRTVs();
	CreateSRVs();
	CreateDSV();
}

void GBufferManager::Finalize()
{
    rtvHeap_.Reset();
    srvHeap_.Reset();
    dsvHeap_.Reset();

    dxManager_ = nullptr;

    delete instance;
    instance = nullptr;
}

void GBufferManager::CreateResources(UINT width, UINT height)
{
	gBufferResources_[(size_t)GBufferType::Albedo] = dxManager_->CreateGBufferResource(width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	gBufferResources_[(size_t)GBufferType::Normal] = dxManager_->CreateGBufferResource(width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
	gBufferResources_[(size_t)GBufferType::Depth] = dxManager_->CreateDepthStencilTextureResource(width, height, DXGI_FORMAT_D24_UNORM_S8_UINT);
}

void GBufferManager::CreateRTVs()
{
    auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    dxManager_->GetDevice()->CreateRenderTargetView(gBufferResources_[(size_t)GBufferType::Albedo].Get(), nullptr, handle);
    handle.ptr += rtvIncrement_;

    dxManager_->GetDevice()->CreateRenderTargetView(gBufferResources_[(size_t)GBufferType::Normal].Get(), nullptr, handle);
}

void GBufferManager::CreateSRVs()
{
    auto device = dxManager_->GetDevice();
    auto handle = srvHeap_->GetCPUDescriptorHandleForHeapStart();

    for (size_t i = 0; i < (size_t)GBufferType::Count; i++)
    {
        // Depthの場合はスキップ
        if (gBufferResources_[i]->GetDesc().Format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
            continue;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Format = gBufferResources_[i]->GetDesc().Format;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(gBufferResources_[i].Get(), &desc, handle);
        handle.ptr += srvIncrement_;
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
    D3D12_GPU_DESCRIPTOR_HANDLE h = srvHeap_->GetGPUDescriptorHandleForHeapStart();
    h.ptr += (size_t)type * srvIncrement_;
    return h;
}