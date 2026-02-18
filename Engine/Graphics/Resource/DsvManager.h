#pragma once
#include <d3d12.h>
#include <cstdint>
#include <wrl.h>
#include <vector>

class DirectXManager;

class DsvManager
{
public:
	DsvManager() = default;
	~DsvManager();

	void Initialize(ID3D12Device* device, UINT descriptorCount = 32);

	void Finalize();

	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(uint32_t index = 0) const { return dsvHandles_[index]; }

	void CreateDsv(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT);

	uint32_t Allocate();
private:
	ID3D12Device* device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

	UINT descriptorSize_ = 0;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvHandles_;

	uint32_t useIndex = 0; 

	uint32_t kMaxCount = 64;
};

