#pragma once
#include <d3d12.h>
#include <stdint.h>
#include <wrl.h>
#include <vector>

class DirectXManager;

class SrvManager
{
public:
	SrvManager() = default;
	~SrvManager();
	// 初期化
	void Initialize(DirectXManager* dxManager);
	// 終了
	void Finalize();
	// 確保
	uint32_t Allocate();
	// 確保したインデックスを返却する。
	// スキンモデルのインスタンスのように生成/破棄を繰り返すものは必ず返すこと。
	// 返さないとヒープ(kMaxCount個)を使い切って Allocate() の assert で落ちる
	void Free(uint32_t index);
	void BeginDraw();
	// SRVの確保が可能かどうかをチェックする関数
	bool CanAllocate() const;
	// 確保済みSRV数（エディタの統計表示用）
	uint32_t GetUsedCount() const { return useIndex - static_cast<uint32_t>(freeList_.size()); }

public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	ID3D12DescriptorHeap* GetHeap() const { return descriptorHeap_.Get(); }

	// SRV生成 (テクスチャ用)
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT MipLevels);
	// SRV生成 (Structured Buffer用)
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	void CreateUAVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

	uint32_t CreateSRVFromResource(ID3D12Resource* pResource, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT mipLevels = 1);
public:
	// 最大SRV数 (最大テクスチャ枚数)
	static const uint32_t kMaxCount;
private:
	DirectXManager* dxManager_ = nullptr;

	// SRV用のデスクリプタサイズ
	static inline uint32_t descriptorSize_;
	// デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;
	// まだ一度も配ったことがない領域の先頭
	uint32_t useIndex = 0;
	// Free() で返却されて再利用できるインデックス
	std::vector<uint32_t> freeList_;
};

