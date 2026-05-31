# リファクタリング記録: リソース管理とパーティクル改良

## 1. メモリリークの修正

### 背景
`PrimitiveFactory::Create()` で生成した `Model` を破棄しても GPU リソースが解放されないリークが確認された。
原因は `Material` と `Mesh` それぞれのデストラクタが GPU バッファを解放していないことと、
`ResourceManager::ReleaseBuffer` が永続マップされたバッファを Unmap せずにステージングへ積んでいたことにある。

---

### 1-1. `Material::~Material()` の修正

**ファイル**: `Engine/3d/Object/Model/Material/Material.h` / `Material.cpp`

**問題**  
`Material::Initialize()` 内で `ResourceManager::CreateUploadBuffer()` を使って確保した
`materialHandle_`（マテリアル定数バッファ）と `materialGBufferHandle_`（GBuffer マテリアルバッファ）が
デストラクタで解放されていなかった。  
また、ハンドルの初期値が `0`（有効ハンドルになりうる値）だった。

**修正内容**

```cpp
// Material.h
uint32_t materialHandle_       = kInvalidBufferHandle;  // 0 → kInvalidBufferHandle
uint32_t materialGBufferHandle_ = kInvalidBufferHandle;  // 0 → kInvalidBufferHandle
```

```cpp
// Material.cpp
Material::~Material()
{
    if (directXManager_) {
        auto* resourceManager = directXManager_->GetResourceManager();
        if (materialHandle_ != kInvalidBufferHandle) {
            resourceManager->ReleaseBuffer(materialHandle_);
            materialHandle_ = kInvalidBufferHandle;
        }
        if (materialGBufferHandle_ != kInvalidBufferHandle) {
            resourceManager->ReleaseBuffer(materialGBufferHandle_);
            materialGBufferHandle_ = kInvalidBufferHandle;
        }
    }
}
```

---

### 1-2. `Mesh` の頂点・インデックスバッファ管理をハンドルベースに移行

**ファイル**: `Engine/3d/Object/Model/Mesh/Mesh.h` / `Mesh.cpp`

**問題**  
`CreateVertexResource()` / `CreateIndexResource()` が `CreateUploadBufferWithData()`（`ComPtr<ID3D12Resource>` を直接返す関数）を使っており、
生成したリソースは `ResourceManager::table_` の追跡対象外だった。  
デストラクタで `ComPtr::Reset()` を直接呼ぶと **GPU フェンス同期なしで即時解放** され、
GPU がそのバッファを参照中であっても破棄されてしまう状態だった（D3D12 デバッグレイヤーが "LIVE OBJECT" を報告）。  
また `vertexHandle_` / `indexHandle_` が `0` 初期化のまま未使用で、`ComPtr` と二重管理になっていた。

**修正内容**

| 変更前 | 変更後 |
|---|---|
| `ComPtr<ID3D12Resource> vertexResource_` / `indexResource_` | 削除 |
| `VertexData* vertexData_` / `uint32_t* indexData_` | 削除 |
| `uint32_t vertexHandle_ = 0` | `BufferHandle vertexHandle_ = kInvalidBufferHandle` |
| `uint32_t indexHandle_ = 0` | `BufferHandle indexHandle_ = kInvalidBufferHandle` |

```cpp
// Mesh.h（追加インクルード）
#include "Graphics/Resource/ResourceManager.h"
```

```cpp
// Mesh.cpp - CreateVertexResource()
void Mesh::CreateVertexResource()
{
    auto* resourceManager = directXManager_->GetResourceManager();
    const size_t vertexSize = sizeof(VertexData) * meshData_.vertices.size();

    vertexHandle_ = resourceManager->CreateUploadBuffer(vertexSize, L"Mesh:Vertex");
    void* ptr = resourceManager->Map(vertexHandle_);
    std::memcpy(ptr, meshData_.vertices.data(), vertexSize);

    ID3D12Resource* resource = resourceManager->GetResource(vertexHandle_);
    vertexBufferView_.BufferLocation = resource->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes    = static_cast<UINT>(vertexSize);
    vertexBufferView_.StrideInBytes  = sizeof(VertexData);
}
```

```cpp
// Mesh.cpp - ~Mesh()
Mesh::~Mesh()
{
    if (directXManager_) {
        auto* resourceManager = directXManager_->GetResourceManager();
        if (vertexHandle_ != kInvalidBufferHandle) {
            resourceManager->ReleaseBuffer(vertexHandle_);
            vertexHandle_ = kInvalidBufferHandle;
        }
        if (indexHandle_ != kInvalidBufferHandle) {
            resourceManager->ReleaseBuffer(indexHandle_);
            indexHandle_ = kInvalidBufferHandle;
        }
    }
}
```

**効果**  
`ReleaseBuffer` は GPU フェンスと連動した遅延解放（staging → pending → 実解放）を行うため、
GPU が使用中のバッファが早期解放されることがなくなる。
スキンメッシュも同じ初期化パスを通るため、併せて修正される。

---

### 1-3. `ResourceManager::ReleaseBuffer()` の Unmap 漏れ修正

**ファイル**: `Engine/Graphics/Resource/ResourceManager.cpp`

**問題**  
`CreateUploadBuffer()` は内部で `Map()` を呼び、リソースを永続マップした状態で保持する。
しかし `ReleaseBuffer()` はステージングへ積む前に `Unmap()` を呼んでおらず、
マップされたままのリソースが解放されていた（D3D12 では Unmap 前に Release することを非推奨としている）。

**修正内容**

```cpp
void ResourceManager::ReleaseBuffer(BufferHandle handle)
{
    if (!IsValidHandle(handle)) return;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto& entry = table_[handle];
        // 永続マップされた Upload バッファは解放前に必ず Unmap する
        if (entry.isUpload && entry.mappedPtr) {
            entry.resource->Unmap(0, nullptr);
            entry.mappedPtr = nullptr;
        }
        staging_.push_back(entry.resource);
        entry.resource.Reset();
        entry.alive = false;
        freeList_.push_back(handle);
    }
    // ...
}
```

**効果**  
`Material` や `Mesh` が `ReleaseBuffer` を呼ぶすべてのケースで、
Unmap → ステージング → フェンス待ち → 実解放 の正しいシーケンスが保証される。

---

## 2. パーティクルの改良: プリミティブ形状のサポート

### 背景
従来のパーティクルは全グループが共有の固定 Plane（非インデックス、6頂点の XY 平面）を
`DrawInstanced(6, instanceCount, 0, 0)` で描画していた。
Ring や Cylinder などの形状をパーティクルに使えるよう拡張した。

---

### 変更概要

| ファイル | 変更内容 |
|---|---|
| `ParticleManager.h` | `PrimitiveRenderer.h` を include。`ParticleGroupGPU` にグループ固有の VB/IB フィールドを追加。`CreateParticleGroup` / `CreateParticleGPU` に `PrimitiveType shape` 引数を追加 |
| `ParticleManager.cpp` | `MeshGenerator.h` を include。`CreateParticleGPU` でグループごとに VB/IB を生成。`Draw` でループ内に `IASetVertexBuffers` / `IASetIndexBuffer` を移動。`Finalize` でグループ VB/IB を `ReleaseBuffer` で解放 |
| `ParticleRenderer.h/.cpp` | `DrawInstanced` → `DrawIndexedInstanced` に変更。引数に `indexCount` を追加 |

---

### `ParticleGroupGPU` への追加フィールド

```cpp
struct ParticleGroupGPU
{
    uint32_t instancingHandle;
    ParticleForGPU* mappedPtr;
    uint32_t srvIndex;
    // 形状ごとの頂点・インデックスバッファ
    BufferHandle vertexHandle = kInvalidBufferHandle;
    BufferHandle indexHandle  = kInvalidBufferHandle;
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    D3D12_INDEX_BUFFER_VIEW  ibv{};
    uint32_t indexCount = 0;
};
```

---

### `CreateParticleGroup` の新シグネチャ

```cpp
// 第3引数を省略すると従来通り Plane（後方互換）
void CreateParticleGroup(
    const std::string name_,
    const std::string textureFilePath,
    PrimitiveType shape = PrimitiveType::Plane
);
```

---

### 形状別メッシュ生成ロジック（`CreateParticleGPU` 内）

```cpp
MeshData meshData;
switch (shape) {
case PrimitiveType::Ring:
    meshData = MeshGenerator::CreateRing();
    break;
case PrimitiveType::Cylinder:
    meshData = MeshGenerator::CreateCylinder();
    break;
case PrimitiveType::Plane:
default:
    // XY平面 (Billboard向け)、旧来仕様と同じ -1〜1 の 2x2 サイズ
    meshData.vertices = {
        {{ 1,  1, 0, 1}, {0, 0}, {0, 0, 1}},
        {{-1,  1, 0, 1}, {1, 0}, {0, 0, 1}},
        {{ 1, -1, 0, 1}, {0, 1}, {0, 0, 1}},
        {{-1, -1, 0, 1}, {1, 1}, {0, 0, 1}},
    };
    meshData.indices = {0, 1, 2, 2, 1, 3};
    break;
}
```

> **注意**: `Plane` は旧来の XY 平面・2×2 サイズを維持しているため、
> 既存のパーティクルエフェクトはそのまま動作する。
> `Ring` / `Cylinder` は `MeshGenerator` の XZ 平面向き（地面に寝た形状）。

---

### 使い方

```cpp
// 従来通り（Plane がデフォルト）
ParticleManager::GetInstance()->CreateParticleGroup("spark", "particle.png");

// リング形状
ParticleManager::GetInstance()->CreateParticleGroup("shockwave", "ring.png", PrimitiveType::Ring);

// シリンダー形状
ParticleManager::GetInstance()->CreateParticleGroup("pillar", "pillar.png", PrimitiveType::Cylinder);
```

---

### 描画フローの変化

```
変更前:
  IASetVertexBuffers(共有 Plane VBV)  ← ループ前に1回だけセット
  for each group:
    DrawInstanced(6, instanceCount, 0, 0)

変更後:
  for each group:
    IASetVertexBuffers(group.vbv)     ← グループごとの形状
    IASetIndexBuffer(group.ibv)
    DrawIndexedInstanced(group.indexCount, instanceCount, 0, 0, 0)
```

---

### `Finalize` でのバッファ解放

```cpp
if (dxManager_) {
    auto* rm = dxManager_->GetResourceManager();
    for (auto& [name, gpu] : particleGPU_) {
        if (gpu.vertexHandle != kInvalidBufferHandle)
            rm->ReleaseBuffer(gpu.vertexHandle);
        if (gpu.indexHandle != kInvalidBufferHandle)
            rm->ReleaseBuffer(gpu.indexHandle);
    }
}
particleGPU_.clear();
```
