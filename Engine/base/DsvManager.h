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

	void Initialize(ID3D12Device* device, UINT descriptorCount = 1);

	void Finalize();

	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(int index = 0) const { return dsvHandles_[index]; }

	void CreateDsv(ID3D12Resource* resource, int index = 0);

private:
	ID3D12Device* device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

	UINT descriptorSize_ = 0;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvHandles_;
};

