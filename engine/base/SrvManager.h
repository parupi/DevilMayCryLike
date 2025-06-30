#pragma once
#include <DirectXManager.h>
class SrvManager
{
public:
	// 初期化
	void Initialize(DirectXManager* dxManager);
	// 終了
	void Finalize();
	// 確保
	uint32_t Allocate();
	void BeginDraw();
	// SRVの確保が可能かどうかをチェックする関数
	bool CanAllocate() const;

public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	// SRV生成 (テクスチャ用)
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT MipLevels);
	// SRV生成 (Structured Buffer用)
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	void SetGraphicsRootDescriptorTable(UINT PootParameterIndex, uint32_t srvIndex);


public:
	// 最大SRV数 (最大テクスチャ枚数)
	static const uint32_t kMaxCount;
private:
	DirectXManager* dxManager_ = nullptr;

	// SRV用のデスクリプタサイズ
	static inline uint32_t descriptorSize_;
	// デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;
	// 次に使用するSRVインデックス
	uint32_t useIndex = 0;
};

