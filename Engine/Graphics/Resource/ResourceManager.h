#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <mutex>

// 小型ハンドル
using BufferHandle = uint32_t;
static const BufferHandle kInvalidBufferHandle = UINT32_MAX;

class CommandContext; // 前方宣言

struct ResourceManagerDesc
{
    ID3D12Device* device = nullptr;
    // commandContext / queue / fence 管理は DirectXManager と調整してください
};


class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager();

    void Initialize(ID3D12Device* device);

    // --- Buffer 管理 ---
    // Upload ヒープで CPU 書き込み可能なバッファを作る（CBV 用など）
    BufferHandle CreateUploadBuffer(size_t sizeInBytes, const std::wstring& debugName = L"");

    // Default ヒープ（GPU 用）。初期データ optional（CPU 側からコピーする実装は別途必要）
    BufferHandle CreateDefaultBuffer(size_t sizeInBytes, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, const std::wstring& debugName = L"");

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBufferWithData(const void* srcData, size_t dataSize);

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadResource(uint64_t size);

    // マップ（Upload バッファのみを想定）。戻り値は nullptr なら失敗
    void* Map(BufferHandle handle);

    // GPU 仮想アドレス取得（CBV用に直接渡せる）
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(BufferHandle handle) const;

    ID3D12Resource* GetResource(BufferHandle handle) { return table_[handle].resource.Get(); }

    // 明示的に破棄（アプリ側がリソースを不要にしたときに呼ぶ）
    // 破棄は遅延解放（staging）となり、OnFrameEnd で fence と紐づいて pending に移る
    void ReleaseBuffer(BufferHandle handle);

    // 描画ループと連携する関数
    // Flush 後に呼ぶ（Signal した fenceValue を渡して staging をペア化）
    void OnFrameEnd(uint64_t fenceValue);

    // GPU が完了した fence を渡し、解放可能なリソースを実際に解放する
    void ProcessPendingReleases(uint64_t completedFenceValue);

    // debug
    void DebugDumpState() const;

    void AddPendingUpload(Microsoft::WRL::ComPtr<ID3D12Resource> resource);
    void ReleasePendingUploads();

private:
    struct Entry
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        void* mappedPtr = nullptr; // Upload heap の Map の結果
        size_t size = 0;
        bool isUpload = false;
        std::wstring debugName;
        bool alive = true;
    };

    BufferHandle AllocHandle();
    bool IsValidHandle(BufferHandle h) const;

    ResourceManagerDesc desc_;

    std::vector<Entry> table_;
    std::vector<BufferHandle> freeList_;
    std::mutex mutex_; // スレッドセーフにしたい場合

    // staged -> pending
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> staging_; // ReleaseBuffer が push
    std::vector<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12Resource>>> pending_; // (fence, res)
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingUploadBuffers_;
};

