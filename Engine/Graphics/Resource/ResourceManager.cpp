#include "ResourceManager.h"
#include <cassert>
#include <cstring>
#include <format>
#include <sstream>

// Helper: debug name取得（GetPrivateData）
static std::wstring GetResourceDebugName(ID3D12Resource* resource)
{
    if (!resource) return L"(null)";
    UINT size = 0;
    HRESULT hr = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, nullptr);
    if (hr != S_OK || size == 0) return L"(no name)";
    std::wstring name; name.resize(size / sizeof(wchar_t));
    hr = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name.data());
    if (FAILED(hr)) return L"(failed)";
    if (!name.empty() && name.back() == L'\0') name.pop_back();
    return name;
}

void ResourceManager::Initialize(ID3D12Device* device)
{
    desc_.device = device;
    assert(desc_.device != nullptr && "Device must be set");
}

ResourceManager::~ResourceManager()
{
    // 最終的に pending をクリアしてリソースを解放
    staging_.clear();
    pending_.clear();
    table_.clear();
}

BufferHandle ResourceManager::AllocHandle()
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (!freeList_.empty()) {
        BufferHandle h = freeList_.back();
        freeList_.pop_back();
        return h;
    }
    BufferHandle h = static_cast<BufferHandle>(table_.size());
    table_.push_back({});
    return h;
}

bool ResourceManager::IsValidHandle(BufferHandle h) const
{
    return (h != kInvalidBufferHandle) && (h < table_.size()) && table_[h].alive;
}

BufferHandle ResourceManager::CreateUploadBuffer(size_t sizeInBytes, const std::wstring& debugName)
{
    assert(desc_.device);
    BufferHandle h = AllocHandle();

    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = sizeInBytes;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = desc_.device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&table_[h].resource)
    );
    assert(SUCCEEDED(hr));

#ifdef _DEBUG
    std::wstring assignedName = debugName.empty() ? (L"UploadBuffer_" + std::to_wstring(h)) : debugName;
    table_[h].resource->SetName(assignedName.c_str());
    table_[h].debugName = assignedName;
#endif

    // Map して保持（Upload は常に map しておくのが便利）
    void* mapped = nullptr;
    table_[h].resource->Map(0, nullptr, &mapped);
    table_[h].mappedPtr = mapped;
    table_[h].size = sizeInBytes;
    table_[h].isUpload = true;
    table_[h].alive = true;

    return h;
}

BufferHandle ResourceManager::CreateDefaultBuffer(size_t sizeInBytes, D3D12_RESOURCE_FLAGS flags, const std::wstring& debugName)
{
    assert(desc_.device);
    BufferHandle h = AllocHandle();

    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = sizeInBytes;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;

    HRESULT hr = desc_.device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&table_[h].resource)
    );
    assert(SUCCEEDED(hr));

#ifdef _DEBUG
    std::wstring assignedName = debugName.empty() ? (L"DefaultBuffer_" + std::to_wstring(h)) : debugName;
    table_[h].resource->SetName(assignedName.c_str());
    table_[h].debugName = assignedName;
#endif

    table_[h].mappedPtr = nullptr;
    table_[h].size = sizeInBytes;
    table_[h].isUpload = false;
    table_[h].alive = true;

    return h;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateUploadBufferWithData(const void* srcData, size_t dataSize)
{
    assert(srcData != nullptr);
    assert(dataSize > 0);

    // ===== Upload Heap の設定 =====
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = dataSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // ===== リソース作成 =====
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadRes;

    HRESULT hr = desc_.device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadRes)
    );
    assert(SUCCEEDED(hr));

    // ===== マップして初期データ転送 =====
    void* mapped = nullptr;
    hr = uploadRes->Map(0, nullptr, &mapped);
    assert(SUCCEEDED(hr));

    std::memcpy(mapped, srcData, dataSize);

    uploadRes->Unmap(0, nullptr);

#ifdef _DEBUG
    static uint64_t uploadCounter = 0;
    std::wstring name = L"UploadBuffer[" + std::to_wstring(uploadCounter++) +
        L"] Size=" + std::to_wstring(dataSize);
    uploadRes->SetName(name.c_str());
#endif

    return uploadRes;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateUploadResource(uint64_t size)
{
    assert(size > 0);

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> upload;

    HRESULT hr = desc_.device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // Upload buffer はこれ
        nullptr,
        IID_PPV_ARGS(&upload)
    );
    assert(SUCCEEDED(hr));

#ifdef _DEBUG
    static uint64_t uploadCounter = 0;
    std::wstring name = L"UploadBufferUseResource[" + std::to_wstring(uploadCounter++) +
        L"] Size=" + std::to_wstring(size);
    upload->SetName(name.c_str());
#endif

    return upload;
}

void* ResourceManager::Map(BufferHandle handle)
{
    if (!IsValidHandle(handle)) return nullptr;
    auto& e = table_[handle];
    if (!e.isUpload) return nullptr; // Default buffer は Map しない
    return e.mappedPtr;
}

D3D12_GPU_VIRTUAL_ADDRESS ResourceManager::GetGPUVirtualAddress(BufferHandle handle) const
{
    if (!IsValidHandle(handle)) return 0;
    auto& e = table_[handle];
    if (!e.resource) return 0;
    return e.resource->GetGPUVirtualAddress();
}

void ResourceManager::ReleaseBuffer(BufferHandle handle)
{
    if (!IsValidHandle(handle)) return;
    // staging に ComPtr を移して、実解放は OnFrameEnd->ProcessPendingReleases に任せる
    {
        std::lock_guard<std::mutex> lk(mutex_);
        staging_.push_back(table_[handle].resource);
        // mark entry dead so nobody else tries to use it
        table_[handle].resource.Reset();
        table_[handle].mappedPtr = nullptr;
        table_[handle].alive = false;
        // add to free list to reuse handle
        freeList_.push_back(handle);
    }

#ifdef _DEBUG
    std::wstring nm = table_[handle].debugName.empty() ? L"(unnamed)" : table_[handle].debugName;
    OutputDebugStringW((L"ResourceManager::ReleaseBuffer -> staging name=" + nm + L" handle=" + std::to_wstring(handle) + L"\n").c_str());
#endif
}

void ResourceManager::OnFrameEnd(uint64_t fenceValue)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (staging_.empty()) return;

    for (auto& res : staging_) {
        pending_.emplace_back(fenceValue, std::move(res));
    }
    staging_.clear();
}

void ResourceManager::ProcessPendingReleases(uint64_t completedFenceValue)
{
    std::lock_guard<std::mutex> lk(mutex_);
#ifdef _DEBUG
    OutputDebugStringA(("ResourceManager::ProcessPendingReleases: completedFence=" + std::to_string(completedFenceValue) + " pendingCount=" + std::to_string(pending_.size()) + "\n").c_str());
#endif
    auto it = pending_.begin();
    while (it != pending_.end()) {
        uint64_t fenceTag = it->first;
#ifdef _DEBUG
        std::wstring name = GetResourceDebugName(it->second.Get());
        OutputDebugStringW((L"  Pending: fenceTag=" + std::to_wstring(fenceTag) + L" name=" + name + L"\n").c_str());
#endif
        if (fenceTag <= completedFenceValue) {
            it = pending_.erase(it);
        } else ++it;
    }
}

void ResourceManager::DebugDumpState() const
{
    std::ostringstream oss;
    oss << "ResourceManager Dump: table size=" << table_.size() << "\n";
    OutputDebugStringA(oss.str().c_str());
}

void ResourceManager::AddPendingUpload(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
    pendingUploadBuffers_.push_back(resource);
}

void ResourceManager::ReleasePendingUploads()
{
    pendingUploadBuffers_.clear();
}
