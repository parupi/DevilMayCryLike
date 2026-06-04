# エンジンリファクタリング記録

## 概要

コードの保守性・拡張性・構造的一貫性の向上を目的とした、エンジン全体にわたるリファクタリング。

---

## ① PSO 生成クラスの責務分割

### 問題
`PSOManager.cpp` が 2138 行に肥大化。14 種類のパイプライン（Sprite, Particle, Object, Animation, OffScreen, Primitive, Skybox, Skinning, Deferred, LightingPath, FinalComposite, Composite, CSM, Trail）のRootSignature・PSO 生成ロジックが全て混在。

### 変更内容

**新規作成**
- `Graphics/Rendering/PSO/PSOCommon.h` — `BlendMode` / `OffScreenEffectType` enum を切り出し
- `Graphics/Rendering/PSO/PSOBuilder/` — パイプラインごとの Desc クラス（14 ペア）

各クラスの責務:
```cpp
class SpritePipeline {
public:
    static ComPtr<ID3D12RootSignature> CreateRootSignature(DirectXManager*);
    static ComPtr<ID3D12PipelineState> CreatePSO(DirectXManager*, ID3D12RootSignature*, BlendMode);
};
```

**変更**
- `PSOManager.cpp` — 2138行 → 約270行。全 Create* メソッドが 1〜2 行の委譲になった
- `CreateXXXSignature()` に `if (!signature_)` ガードを追加（同一PSO を複数BlendMode で生成しても RootSignature が重複生成されないバグを修正）

---

## ② IRenderPass インターフェースと描画パス分離

### 問題
`RenderPipeline::Execute()` が 100行超の巨大メソッドで、GBuffer・Lighting・Forward・Shadow の全ロジックが混在。新しい描画パスを追加するたびに `RenderPipeline` 本体の変更が必要だった。

### 変更内容

**新規作成**
- `Graphics/Rendering/RenderPath/IRenderPass.h` — `Execute()` と `GetName()` の純粋仮想インターフェース
- `Graphics/Rendering/RenderPath/IRenderPass.h` 内 `SharedRenderTarget` 構造体 — パス間で共有する RTV/DSV リソースのビュー
- `Graphics/Rendering/RenderPath/Pass/` — 4つの具体パスクラス

| クラス | 役割 |
|---|---|
| `ShadowRenderPass` | シャドウマップ生成 |
| `GBufferRenderPass` | GBuffer 書き込み（Deferred） |
| `LightingRenderPass` | ライティング計算（Deferred） |
| `ForwardSceneRenderPass` | フォワード描画 + シーン全体 |

**変更**
- `RenderPipeline.h/.cpp` — `passes_` (`vector<unique_ptr<IRenderPass>>`) を順番に呼ぶだけになった

```cpp
void RenderPipeline::Execute() {
    for (auto& pass : passes_) { pass->Execute(); }
    // OffScreen / Final 合成...
}
```

新しいパスは `passes_.push_back()` するだけで追加可能。

---

## ③ BaseRenderer ISP 分離

### 問題
`BaseRenderer` の `DrawGBuffer()` と `DrawShadow()` が純粋仮想だったため、これらを必要としない `InstancingRenderer`（両方空スタブ）や `PrimitiveRenderer`（DrawShadow のみ空スタブ）も実装を強制されていた（インターフェース分離の原則違反）。

### 変更内容

**新規作成**
- `World3D/Object/Renderer/IDeferredDrawable.h` — `DrawGBuffer()` インターフェース
- `World3D/Object/Renderer/IShadowCaster.h` — `DrawShadow()` インターフェース

**変更**

| クラス | 変更内容 |
|---|---|
| `BaseRenderer` | `DrawGBuffer` / `DrawShadow` を削除。フォワード描画のみの責務に |
| `ModelRenderer` | `IDeferredDrawable + IShadowCaster` を継承 |
| `PrimitiveRenderer` | `IDeferredDrawable` のみ継承 |
| `InstancingRenderer` | 空スタブの `DrawGBuffer` / `DrawShadow` を完全削除 |
| `Object3d` | `deferredDrawables_` / `shadowCasters_` リストを追加。`AddRenderer()` 時に `dynamic_cast` でリストを構築し、毎フレームの `dynamic_cast` を回避 |
| `Object3d::ResetObject()` | `deferredDrawables_` / `shadowCasters_` のクリア追加（バグ修正） |

---

## ④ Singleton の `unique_ptr` 化

### 問題
全 18 個のシングルトンが `static T* instance` + 手動 `delete instance` で管理されており、所有権が不明確だった。また `T(T&) = default` によりコピーが誤って許可されていた。

### 変更内容（全 18 クラス共通）

| 変更箇所 | 変更前 | 変更後 |
|---|---|---|
| 静的メンバ | `static T* instance;` | `static unique_ptr<T> instance;` |
| コピーコンストラクタ | `T(T&) = default;`（コピー可能） | `T(const T&) = delete;` |
| コピー代入 | `T& operator=(T&) = default;` | `T& operator=(const T&) = delete;` |
| デストラクタ | `private: ~T() = default;` | 宣言を削除（暗黙の `public` デストラクタになり `unique_ptr` が使用可能に） |
| .cpp 静的定義 | `T* T::instance = nullptr;` | `unique_ptr<T> T::instance;` |
| GetInstance() | `instance = new T();` + `return instance;` | `instance.reset(new T());` + `return instance.get();` |
| Finalize() | `delete instance; instance = nullptr;` | `instance.reset();` |

**対象クラス（18個）:** Object3dManager, RendererManager, CollisionManager, CameraManager, LightManager, SceneManager, SceneTransitionController, TransitionManager, Input, Audio, TextureManager, SpriteManager, ParticleManager, ModelManager, PrimitiveLineDrawer, ImGuiManager, SkySystem, OffScreenManager

**特殊ケース**
- `SpriteManager` — static メンバが `public` に露出していたのを `private` に修正
- `ParticleManager` — `once_flag` を使わないため誤追加した宣言を除去
- `ModelRenderer::camera_` — 宣言時の `CameraManager::GetInstance()` 呼び出しを `nullptr` 初期化に変更（`Update()` で毎フレーム更新されるため安全）

---

## ⑤ DirectXManager の責務分割

### 問題
DirectXManager がシェーダーコンパイル・FPS制御・リソース遅延解放の 3 系統の責務を持ち、かつ `ResourceManager` と重複した dead code が存在していた。

### 変更内容

**新規作成**
- `Graphics/Device/ShaderCompiler.h/.cpp` — DXC コンパイラ責務を抽出

```cpp
class ShaderCompiler {
public:
    void Initialize();
    IDxcBlob* Compile(const std::wstring& filePath, const wchar_t* profile);
};
```

- `Graphics/Device/FrameTimer.h/.cpp` — FPS 固定責務を抽出

```cpp
class FrameTimer {
public:
    void Initialize();
    void Update(); // EndDraw() の末尾から呼ぶ
};
```

**DirectXManager から削除（dead code）**
- `pendingRelease_[]` / `pendingReleases_` / `pendingReleaseStaging_` — `ResourceManager` が同等機能を持ち、`EndDraw()` では呼ばれていなかった
- `RegisterResourceForRelease()` / `OnBeginFrame()` / `MoveStagingToPending()` / `ProcessPendingReleases()` / `GetResourceDebugName()`

**行数変化:** .h: 162行 → 100行、.cpp: 520行 → 281行（**約320行削減**）

---

## ⑥ EngineContext による依存性注入（DI）

### 問題
`RenderPipeline` と各 Pass クラスの内部で `GetInstance()` が多数呼ばれており、エンジン内部がシングルトンに直接依存していた。

### 変更内容

**新規作成**
- `Core/EngineContext.h` — 全サービスへの**非所有ビュー**構造体

```cpp
struct EngineContext {
    DirectXManager*      dxManager;
    PSOManager*          psoManager;
    Object3dManager*     object3dManager;
    LightManager*        lightManager;
    CameraManager*       cameraManager;
    SkySystem*           skySystem;
    OffScreenManager*    offScreenManager;
    SceneManager*        sceneManager;
    SpriteManager*       spriteManager;
    TransitionManager*   transitionManager;
    CollisionManager*    collisionManager;
    PrimitiveLineDrawer* primitiveLineDrawer;
    ImGuiManager*        imGuiManager; // DEBUG only
};
```

**変更**

| ファイル | 変更内容 |
|---|---|
| `Core/GuchisFramework.h` | `EngineContext ctx_` を `protected` に追加 |
| `Core/GuchisFramework.cpp` | `Initialize()` でコアサービスを `ctx_` に登録 |
| `Application/MyGameTitle.cpp` | 全サービス初期化後に `ctx_` を完成させ、`renderPipeline_->Initialize(ctx_)` を呼ぶ |
| `RenderPipeline` | `Initialize(const EngineContext&)` に変更。`Execute()` は引数なし |
| 全 Pass クラス | `Initialize(const EngineContext&, ...)` に変更。`GetInstance()` 呼び出しを全廃 |
| `ShadowPass` | `Object3dManager*` を DI で受け取るよう変更 |

**Strangler Fig アプローチ:** エンジン内部は DI で動作、ゲームコードは引き続き `GetInstance()` を使用可能。`RenderPath` 以下の `GetInstance()` 呼び出しは **0件** になった。

---

## ⑦〜⑩ ファイル構成の整理

### ⑦ スペルミス修正: `debuger/` → `Debugger/`

**更新ファイル数:** 14 ファイル

### ⑧ `PSO/Pipeline/` → `PSO/PSOBuilder/`

`Pipeline` という名前が `RenderPipeline` と混同しやすいため、用途を表す `PSOBuilder` に変更。

**更新ファイル数:** PSOManager.cpp の 14 インクルード

### ⑨ `core/MyGameTitle` → `Application/`

エンジン基盤（`Core/`）とアプリケーション層（`Application/`）の境界を明確化。

**更新ファイル数:** main.cpp の 1 インクルード

### ⑩ `math/function.h` → `Math/MathUtils.h`

何の関数か不明な名前を改善。

**更新ファイル数:** 25 ファイル

---

## ⑪ Shadow の統合

### 変更内容

```
変更前:
Graphics/Rendering/Shadow/          ← CascadedShadowMap（アルゴリズム）
Graphics/Rendering/RenderPath/Shadow/ ← ShadowPass（パスラッパー）

変更後:
Graphics/Rendering/Shadow/          ← 全シャドウ関連を統合
├── CascadedShadowMap.h/.cpp
├── ShadowTypes.h
└── ShadowPass.h/.cpp
```

---

## ⑫ RenderPath 内部の抽象レベル整理

### 変更内容

```
変更前:
RenderPath/
├── Pass/           ← 実行単位
├── Deferred/       ← 実装詳細（名前が実装依存）
├── Shadow/         ← 実装詳細
└── ForwardRenderPath.h  ← ルートに散在

変更後:
RenderPath/
├── IRenderPass.h
├── RenderPipeline.h/.cpp
├── Pass/           ← 高レベル（IRenderPass 実装）
└── Impl/           ← 低レベル（実装詳細）
    ├── GBufferManager.h/.cpp
    ├── GBufferPath.h/.cpp
    ├── LightingPath.h/.cpp
    └── ForwardRenderPath.h/.cpp
```

---

## ⑬ レンダリング関連コードの `Graphics/Rendering/` への統合

### 変更内容

| 移動元 | 移動先 | ファイル数 |
|---|---|---|
| `offscreen/` | `Graphics/Rendering/PostEffect/` | 14 |
| `3d/SkySystem/` | `Graphics/Rendering/Sky/` | 2 |
| `3d/Trail/` | `Graphics/Rendering/Effect/` | 2 |
| `2d/` | `Graphics/Rendering/Sprite/` | 5 |
| `base/Particle/` | `Graphics/Rendering/Particle/` | 15 |

**更新ファイル数:** 90 ファイル

---

## ⑭ `base/` の解体

| 移動元 | 移動先 | 理由 |
|---|---|---|
| `base/Logger.h/.cpp` | `Utility/Logger.h/.cpp` | 汎用ユーティリティ |
| `base/StringUtility.h/.cpp` | `Utility/StringUtility.h/.cpp` | 汎用ユーティリティ |
| `base/utility/DeltaTime.h/.cpp` | `Utility/DeltaTime.h/.cpp` | 汎用ユーティリティ |
| `base/WindowManager.h/.cpp` | `Platform/WindowManager.h/.cpp` | プラットフォーム管理 |

`base/` ディレクトリは完全に削除。**更新ファイル数:** 90 ファイル（⑬と同時処理）

---

## ⑮ ディレクトリ名の PascalCase 統一

| 変更前 | 変更後 | 備考 |
|---|---|---|
| `audio/` | `Audio/` | |
| `core/` | `Core/` | |
| `debugger/` | `Debugger/` | |
| `input/` | `Input/` | |
| `math/` | `Math/` | |
| `scene/` | `Scene/` | |
| `3d/` | `World3D/` | 数字始まりはPascalCase不可のため意味のある名前に |

**更新ファイル数:** 77 ファイル（`Externals/` は除外）

> **注意:** PowerShell の `-ne` はデフォルトで大文字小文字を区別しないため、インクルード置換には `-cne` を使用した。

---

## 最終的なディレクトリ構造

```
Engine/
├── Application/          ← ゲーム固有クラス (MyGameTitle)
├── Audio/                ← オーディオシステム
├── Core/                 ← エンジン基盤 (GuchisFramework, EngineContext)
├── Debugger/             ← デバッグツール (ImGuiManager, GlobalVariables)
├── Graphics/
│   ├── Device/           ← DirectXManager, CommandContext, ShaderCompiler, FrameTimer
│   ├── Rendering/
│   │   ├── Effect/       ← WeaponTrail
│   │   ├── Particle/     ← ParticleManager 等
│   │   ├── PostEffect/   ← OffScreenManager, 各エフェクト
│   │   ├── PSO/
│   │   │   └── PSOBuilder/ ← 各パイプライン Desc クラス (14種)
│   │   ├── RenderPath/
│   │   │   ├── Impl/     ← GBufferPath, LightingPath, ForwardRenderPath
│   │   │   └── Pass/     ← IRenderPass 実装 (Shadow/GBuffer/Lighting/Forward)
│   │   ├── Shadow/       ← CascadedShadowMap, ShadowPass
│   │   ├── Sky/          ← SkySystem
│   │   └── Sprite/       ← Sprite, SpriteManager
│   └── Resource/         ← SrvManager, RtvManager, DsvManager 等
├── Input/                ← 入力システム
├── Math/                 ← 数学ライブラリ (Vector, Matrix, MathUtils)
├── Platform/             ← プラットフォーム層 (WindowManager)
├── Scene/                ← シーン管理
├── Utility/              ← 汎用ユーティリティ (Logger, DeltaTime, StringUtility)
└── World3D/              ← 3D ワールドコンポーネント
    ├── Camera/
    ├── Collider/
    ├── Light/
    ├── Object/           ← Object3d, Model, Renderer 等
    └── Primitive/
```

---

## 変更規模サマリー

| カテゴリ | 変更内容 |
|---|---|
| 新規作成ファイル | 約 60 ファイル |
| 修正ファイル | 約 250 ファイル |
| 削除コード | 約 2000 行（dead code 含む） |
| ディレクトリ移動 | 15 ディレクトリ |
| インクルードパス更新 | 延べ約 400 箇所 |
