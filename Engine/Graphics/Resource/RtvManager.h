#pragma once
#include <d3d12.h>
#include <stdint.h>
#include <wrl.h>

class DirectXManager;

class RtvManager
{
public:
	RtvManager() = default;
	~RtvManager();
	// RTVの最大数
	static const uint32_t kMaxCount = 128;

	// 初期化処理
	void Initialize(DirectXManager* dxManager);
	// 終了処理
	void Finalize();

	uint32_t Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);

	void CreateRTV(uint32_t rtvIndex, ID3D12Resource* pResource);

	bool CanAllocate() const { return useIndex_ < kMaxCount; }

private:
	DirectXManager* dxManager_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;

	static inline uint32_t descriptorSize_ = 0;

	uint32_t useIndex_ = 0;
};

