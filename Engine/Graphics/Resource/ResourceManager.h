#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <memory>

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
    /// <summary>
    /// アップロードバッファの書き方。CPU と GPU を並行して走らせる（GPU が前のフレームを描いている間に
    /// CPU が次のフレームを書く）ので、GPU が読んでいる最中のメモリへ書かないための区別。
    /// </summary>
    enum class UploadUsage {
        // 既定。Map が返すのは CPU 側の写しで、提出の直前（前のフレームの GPU が終わった後）に丸ごと GPU へ写す。
        // 毎フレーム書き換える小さな定数バッファなど
        Dynamic,
        // 作ったときに書くだけのもの（メッシュの頂点など）。Map は GPU のメモリを直接返す。
        // 描画に使い始めた後に書き換えると、GPU が読んでいる最中に書くことになるので注意
        Static,
        // 大きくて毎フレーム一部だけ書くもの（パーティクル）。Map は CPU 側の写しを返し、
        // 書いた範囲を MarkDirty で知らせたぶんだけ提出の直前に写す
        Ranged,
    };

    // Upload ヒープで CPU 書き込み可能なバッファを作る（CBV 用など）
    BufferHandle CreateUploadBuffer(size_t sizeInBytes, const std::wstring& debugName = L"", UploadUsage usage = UploadUsage::Dynamic);

    // UploadUsage::Ranged のバッファで、このフレームに書いた範囲を知らせる（重なっていてよい）
    void MarkDirty(BufferHandle handle, size_t offset, size_t size);

    // CPU 側の写しを GPU のバッファへ写す。GPU がそのバッファを読み終えている（＝前に提出したものが
    // すべて完了している）ときに、提出の直前に呼ぶこと（CommandContext の提出前コールバック）
    void SyncUploads();

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
    // GPU の完了を待った後に呼ぶ。今あるコピー元をすべて捨てる（ロード中の FlushUploads 用）
    void ReleasePendingUploads();
    // フレームの終わりに呼ぶ。コピー元を「このフェンスが終わったら捨てる」へ回す。
    // CPU と GPU が並行して走るので、提出した直後はまだ GPU がコピーしている最中かもしれない
    void RetirePendingUploads(uint64_t fenceValue);
    // 描画で使っているかもしれないリソースを、フェンスが終わるまで生かしておく（作り直す前の古いテクスチャなど）
    void DeferRelease(Microsoft::WRL::ComPtr<ID3D12Resource> resource) { AddPendingUpload(std::move(resource)); }

private:
    struct AlignedFree {
        void operator()(uint8_t* p) const;
    };

    struct Entry
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        void* mappedPtr = nullptr; // Upload heap の Map の結果（GPU が読むメモリ）
        size_t size = 0;
        bool isUpload = false;
        std::wstring debugName;
        bool alive = true;

        UploadUsage usage = UploadUsage::Static;
        // Dynamic / Ranged の CPU 側の写し。Map はこれを返す
        std::unique_ptr<uint8_t, AlignedFree> shadow;
        // Ranged で、このフレームに書かれた範囲 [dirtyBegin, dirtyEnd)
        size_t dirtyBegin = 0;
        size_t dirtyEnd = 0;
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

    // SyncUploads で写す対象（Dynamic / Ranged）のハンドル
    std::vector<BufferHandle> syncList_;
};

