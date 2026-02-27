#include "DsvManager.h"
#include "Graphics/Device/DirectXManager.h"

DsvManager::~DsvManager()
{
	Finalize();
}

void DsvManager::Initialize(ID3D12Device* device, UINT descriptorCount)
{
	device_ = device;

	// heap生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.NumDescriptors = descriptorCount;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap_));
	dsvHeap_->SetName(L"DSVHeap");

	descriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	dsvHandles_.resize(descriptorCount);

	auto handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < descriptorCount; ++i) {
		dsvHandles_[i] = handle;
		handle.ptr += descriptorSize_;
	}
}

void DsvManager::Finalize()
{
	if (dsvHeap_) {
		dsvHeap_.Reset();
	};
	dsvHandles_.clear();
	device_ = nullptr;
	descriptorSize_ = 0;
}

void DsvManager::CreateDsv(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = format;

	device_->CreateDepthStencilView(resource, &dsvDesc, dsvHandles_[index]);
}

uint32_t DsvManager::Allocate()
{
	assert(useIndex < kMaxCount);

	// returnする番号を一旦記録しておく
	int index = useIndex;
	// 次回のために番号を1進める
	useIndex++;
	// 上で記録した番号を返す
	return index;
}