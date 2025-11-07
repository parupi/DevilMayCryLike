#include "RtvManager.h"
#include <base/DirectXManager.h>

RtvManager::~RtvManager()
{
	Finalize();
}

void RtvManager::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;
	// ヒープの生成
	descriptorHeap_ = dxManager_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kMaxCount, false);
	// ディスクリプタのサイズをRTVに設定
	descriptorSize_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void RtvManager::Finalize()
{
	if (descriptorHeap_) {
		descriptorHeap_.Reset();
	};
	useIndex_ = 0;
	dxManager_ = nullptr;
}

uint32_t RtvManager::Allocate()
{
	assert(useIndex_ < kMaxCount);
	return useIndex_++;
}

D3D12_CPU_DESCRIPTOR_HANDLE RtvManager::GetCPUDescriptorHandle(uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += descriptorSize_ * index;
	return handle;
}

void RtvManager::CreateRTV(uint32_t rtvIndex, ID3D12Resource* pResource)
{
	dxManager_->GetDevice()->CreateRenderTargetView(pResource, nullptr, GetCPUDescriptorHandle(rtvIndex));
}
