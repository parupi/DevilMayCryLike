# エディタ再構成プラン

> **進捗**: Phase 0〜6 すべて完了（2026-08-03）。
> 完成形の説明は [`EditorArchitecture.md`](EditorArchitecture.md) にある。こちらは経緯の記録。

やりたいことは2つ。

1. **エンジンの各種機能をエディタ側から呼び出せるようにする**
   （今は「エンジンが自分のUIを描く」。これを「エディタがエンジンAPIを叩く」に反転させる）
2. **アプリのエディタとエンジンのエディタを分ける**

この2つは同じ話の裏表になっている。依存の向きを正すと、自然に分離もできる。

---

## 1. 現状

### 置き場所

| 場所 | 中身 |
|---|---|
| `Engine/Debugger/Editor*.{h,cpp}` | シェル（Registry / MenuBar / DebugDraw / Layout / Stats / GameView）約1,000行 |
| `Engine/Debugger/ImGuiManager.{h,cpp}` | ImGuiのDX12バックエンド + SRVヒープ + フレーム駆動 |
| エンジン各所に散在 | `LightManager::DrawLightEditor` / 各PostEffect の `DrawImGui` / `CascadedShadowMap` / `ParticleEditor` / `TimeManager` / `DeltaTime` |
| App各所に散在 | `Player::DebugGui` / `Enemy` / `PlayerCombat`（Attack Editor）/ `GameCamera::DrawCameraEditor` / `StylishScoreManager` / `HitEffectSystem` / `Ground` / `Prop` |

`EditorWindow::Begin` の呼び出し口は約30。うち **App側が14ファイル、エンジン側が13ファイル**。

### 問題点

**(a) 依存が逆流している**
`LightManager.cpp` や `VignetteEffect.cpp` といったランタイムのクラスが `Debugger/EditorWindowRegistry.h` を include している。
エンジンのランタイム → エディタ、という向きの依存が27ファイル分ある。
このせいで「エディタから機能を呼ぶ」ことができない（呼ばれる側がUIを持ってしまっている）。

**(b) エンジン機能の大半にエディタからの入口がない**
UIを持っているのは Light / PostEffect / Shadow / Particle / Time だけ。
`Object3dManager`（オブジェクト一覧・削除）、`ModelManager`（ロード・一覧）、`TextureManager`、
`CameraManager`（切替）、`OffScreenManager`（エフェクトの並び順）、`Audio`、`CollisionManager` には
エディタから触る手段がない。**ここが「エンジン機能をエディタから呼びたい」の実体。**

**(c) エンジンのエディタがAppを知ってしまっている**
`EditorLayout.cpp` のプリセットに `"Player"` `"Enemy"` `"Attack Editor"` `"Stylish"` `"GameCamera"` が
文字列で直書きされている。コード上のincludeはないが、知識としてApp依存。
また `GameScene::DebugUpdate()` が `player_->DebugGui()` を手書きで呼んでいるので、
Appのエディタ対象が増えるたびにシーンを編集することになる。

---

## 2. 目指す形

### 依存の向き（これだけは絶対に守る）

```
    App/Editor  ──────▶  Engine/Editor  ──────▶  Engine ランタイム
        │                                            ▲
        └────────────────────────────────────────────┘
                    App ランタイム
```

- `Engine/Editor` は **App を include しない**
- `Engine` ランタイムは **Engine/Editor を include しない**（← 今ここが破れている）
- 合流点は `MyGameTitle`（既にAppとEngineの両方を知っている唯一の場所）

### ディレクトリ

```
Engine/Editor/
  Core/
    EditorContext.{h,cpp}        ← エンジンサービスへの窓口（新規）
    EditorHost.{h,cpp}           ← 拡張ポイント + Draw()の集約（新規／旧MenuBarを吸収）
    EditorWindowRegistry.{h,cpp} ← 移動
    EditorMenuBar.{h,cpp}        ← 移動
    EditorLayout.{h,cpp}         ← 移動（プリセットを登録制に）
    EditorDebugDraw.{h,cpp}      ← 移動
    EditorStats.{h,cpp}          ← 移動
    EditorGameView.{h,cpp}       ← 移動
    EditorSelection.{h,cpp}      ← HierarchyとInspectorをつなぐ（新規）
  Windows/
    HierarchyWindow.cpp          ← Object3dManager
    InspectorWindow.cpp          ← 選択中オブジェクトのTransform/Material
    AssetBrowserWindow.cpp       ← ModelManager / TextureManager
    LightWindow.cpp              ← LightManagerから移設
    CameraWindow.cpp             ← CameraManager
    RenderWindow.cpp             ← OffScreenManager / CSM / SkySystem
    ParticleWindow.cpp           ← 既存ParticleEditorを移設
    AudioWindow.cpp / CollisionWindow.cpp / ProfilerWindow.cpp

Engine/Debugger/
    ImGuiManager.{h,cpp}         ← 残す（DX12バックエンドであってエディタではない）
    GlobalVariables / LeakChecker ← 残す

App/Editor/
  AppEditor.{h,cpp}              ← 登録の入口。MyGameTitleから1回呼ぶ
  AppEditorLayouts.cpp           ← 「バトル調整」などAppのプリセット
  Windows/
    PlayerEditorWindow.cpp / EnemyEditorWindow.cpp / AttackEditorWindow.cpp
    CameraWorkWindow.cpp / StylishWindow.cpp / HitEffectWindow.cpp / StageWindow.cpp
```

---

## 3. 実装手順

各フェーズの終わりで必ずビルドが通る粒度で切ってある。

### Phase 0 — 引っ越しだけ（中身は一切変えない）✅ 完了

`Engine/Debugger/Editor*.{h,cpp}` の6組を `Engine/Editor/Core/` へ移動し、
include を `Debugger/Editor*.h` → `Editor/Core/Editor*.h` に一括置換（27ファイル）。
`premake5.lua` は `Engine/**` グロブなので、premake5 の再実行だけで済む。

`ImGuiManager` / `GlobalVariables` / `LeakChecker` は `Engine/Debugger/` に残した。
ImGuiManager はエディタではなく ImGui の DX12 バックエンドなので、移すと責務がぼやける。

---

### Phase 1 — 拡張ポイントを作る（App分離の土台）✅ 完了

`Engine/Editor/Core/EditorHost.{h,cpp}` を新設。これ1枚 include すれば拡張できる
（`EditorContext.h` / `EditorLayout.h` / `EditorWindowRegistry.h` を巻き込んである）。

```cpp
namespace Editor {
    using DrawFunc = std::function<void()>;

    void AddWindowDrawer(DrawFunc drawer);          // 毎フレーム呼ばれる描画関数
    void AddMenu(const char* label, DrawFunc body); // メニューバーに独自メニュー
    void AddLayoutPreset(LayoutPreset preset);      // 宣言は EditorLayout.h

    void Initialize();   // 設定ロード + エンジン標準ウィンドウ／プリセットの登録
    void Finalize();     // 設定セーブ + drawer の破棄
    void Draw();         // DockSpace・メニューバー・全ウィンドウ
}
```

あわせて実施したこと:

- **`ImGuiManager::Begin()` が `Editor::Draw()` 1本になった**。
  DockSpace 構築・`EditorLayout::ApplyPendingPreset`・メニューバー・Debug Log は
  すべてエディタ側へ移動し、`ImGuiManager::SetupDockSpace()` は削除。
- **Debug Log を `Engine/Editor/Windows/DebugLogWindow.{h,cpp}` へ切り出した**。
  Phase 3 で作るウィンドウ群の1つ目にあたる。
- **Game ビューは drawer として登録する**。オフスクリーンを持っているのは ImGuiManager なので、
  `Editor::AddWindowDrawer([this]{ gameView_.DrawWindow(); })` で預ける形にした。
  `this` をキャプチャしているので、`Editor::Finalize()` で ImGui の破棄より先に drawer を捨てている。
- **`EditorLayout` を固定配列 → 登録制に変えた**。`enum class Preset` は廃止して添字で扱う。
  ウィンドウ名は `const char*` から `std::string` へ（App から動的に足せるようにするため）。
- **`AddMenu` は同じ label で二度呼ぶと同一メニューに連結される**。
  Engine と App が同じ "Game" メニューに項目を足せるようにするため。

---

### Phase 2 — エディタからエンジンを引けるようにする（EditorContext）✅ 完了

`EngineContext` に7つ追加した:
`winManager / modelManager / textureManager / particleManager / rendererManager / audio / input`

`Engine/Editor/Core/EditorContext.{h,cpp}` で受け取る:

```cpp
namespace Editor {
    void SetContext(const EngineContext& context);
    bool HasContext();
    const EngineContext& Ctx();   // 未設定でも全メンバ nullptr の実体が返る
}
```

`MyGameTitle::Initialize()` で `ctx_` を埋め終わった直後に `Editor::SetContext(ctx_)`。

**注意**: `ImGuiManager::Initialize()`（＝ `Editor::Initialize()`）はそれより前に走る。
`Ctx()` 自体は未設定でも安全に返るが、**ウィンドウ側は必ずポインタの null チェックをすること**。

```cpp
if (auto* objects = Editor::Ctx().object3dManager) {
    for (Object3d* obj : objects->GetAllObject()) { ... }
}
```

---

### Phase 5 の先取り — `App/Editor/` の骨組み ✅ 完了

Phase 1 でプリセットを登録制にした結果、`"Player"` `"Attack Editor"` `"Stylish"` といった
App のウィンドウ名をエンジン側に残せなくなった。そこで `App/Editor/AppEditor.{h,cpp}` を先に作り、
`MyGameTitle::Initialize()` の `Editor::SetContext()` の直後から `AppEditor::Register()` を呼んでいる。

現在のプリセットの内訳:

| プリセット | 登録元 |
|---|---|
| 標準 / VFX作業 | `EditorLayout::RegisterBuiltinPresets()`（エンジン） |
| バトル調整 / カメラ調整 / VFX作業(ゲーム) | `AppEditor::Register()`（App） |

エンジンの「標準」は Game / Light Manager / Debug Log / TimeManager / DeltaTime だけになった。
従来そこにあった Player・Enemy・GameCamera は App 側の「バトル調整」「カメラ調整」が担当する。

ウィンドウ本体の移設は Phase 5 のまま。`AppEditor::Register()` に TODO を置いてある。

**include の注意**: includedirs に `Engine` と `App` の両方が入っているので、
`<Editor/AppEditor.h>` は Engine/Editor に無いことを確認してから App 側が引かれる
（既存の `<Scene/SceneFactory.h>` と同じ仕組み）。
**App/Editor に Engine/Editor と同名のファイルを置かないこと。**

---

### Phase 3 — エンジン機能のウィンドウを作る（本命）

新規に `Engine/Editor/Windows/` へ足していく。既存の `DrawImGui` はまだ消さない（並走させる）。

全部 `Engine/Editor/Windows/` に置き、`Editor::Initialize()` で drawer として登録している。

| ウィンドウ | 呼ぶエンジンAPI | できること |
|---|---|---|
| **Hierarchy** | `Object3dManager::GetAllObject / AddObject / DeleteObject`、`ModelManager::LoadModel`、`RendererManager::AddRenderer` | 一覧・絞り込み・選択・複製・削除・**生成**（モデル/プリミティブ/空） |
| **Inspector** | `Object3d::GetWorldTransform / GetRenderers / GetColliders / GetOption`、`BaseRenderer::DebugGui` | Transform / 描画設定 / Renderer / Collider |
| **Asset Browser** | `ModelManager::models / skinnedModels`、`TextureManager` | モデル一覧＋読み込み＋その場配置、テクスチャ一覧＋プレビュー |
| **Light Manager** | `LightManager::GetLights / AddLight / RemoveLight` | 一覧・追加（Directional/Point/Spot）・個別編集・削除 |
| **Render** | `OffScreenManager::GetEffects / MoveEffect`、`CascadedShadowMap`、`SkySystem` | ポストエフェクトの**適用順入れ替え**とON/OFF、CSM、スカイ |
| **Camera** | `CameraManager::GetCameraNames / SetActiveCamera / FindCamera` | 一覧・アクティブ切替（補間時間つき）・位置/回転/FOV/near/far |
| **Audio** | `Audio::SoundLoadWave / SoundPlayWave / StopBGM / SetBGMVolume` | Resource/sound の走査・読み込み・試聴・音量 |
| **Profiler** | 各Manager + `SrvManager::GetUsedCount` | Object3d/Renderer/Collider/Light/Model/Texture/Particle の数、SRV使用率 |

#### 追加したファイル

| ファイル | 役割 |
|---|---|
| `Engine/Editor/Core/EditorSelection.{h,cpp}` | 選択の保持。**名前(std::string)だけ**を持つ |
| `Engine/Editor/Core/EditorAssetUtil.{h,cpp}` | `ScanModelFolders()` と `FindLoadedModelName()` |
| `Engine/Editor/Windows/*.{h,cpp}` | 上表の8ウィンドウ + `DebugLogWindow` |

#### エンジンランタイム側に足したもの（ゲッターとごく小さな追加だけ）

| クラス | 追加 |
|---|---|
| `Object3d` | `GetRenderers()` / `GetColliders()` / `GetIsDraw()` |
| `LightManager` | `GetLights()`。**`DrawLightEditor()` は削除**して LightWindow へ移した |
| `CameraManager` | `GetCameraNames()` / `GetActiveCameraName()` |
| `TextureManager` | `GetLoadedTextureNames()` / `GetLoadedTextureCount()` / `GetResource()` / `TryGetMetaData()` |
| `SrvManager` | `GetUsedCount()` |
| `Audio` | `GetSoundDataMap()`（const参照版） |
| `OffScreenManager` | `MoveEffect(index, direction)` |
| `BaseOffScreen` | `SetActive()` を基底へ集約（派生5クラスの重複を削除） |

**`LightManager::Update()` から ImGui 呼び出しが消えた**（`DrawLightEditor()` と `csm->DrawDebugUI()`）。
CSM のUIは CascadedShadowMap 自身が持ったままで、RenderWindow から呼んでいる。

#### 押さえた設計上の点

**Selection はポインタで持たない**。`Object3d*` を握ると `RemoveDeadObject()` で実体が消えた瞬間に
ぶら下がる。名前だけ持って毎フレーム引き直す。
このとき **`Object3dManager::FindObject()` は使わない** — 見つからないと `Logger::Log` を吐くので、
選択中のオブジェクトが消えた瞬間に Debug Log が溢れる。`GetAllObject()` を自前で回す。

**生成は「作って登録する」だけ**。`Object3d` のコンストラクタが `Initialize()` まで済ませる:

```cpp
auto object = std::make_unique<Object3d>(uniqueName);   // ctor が Initialize まで呼ぶ
Object3d* raw = object.get();
manager->AddObject(std::move(object));

models->LoadModel(modelName);                            // 未読み込みならここで
auto renderer = std::make_unique<ModelRenderer>(raw->name_, modelName);
BaseRenderer* rawRenderer = renderer.get();
renderers->AddRenderer(std::move(renderer));             // 所有は RendererManager
raw->AddRenderer(rawRenderer);                           // Object3d は生ポインタで参照
```

- `RendererManager::FindRender(name)` で引き直さない。同名衝突で他人のレンダラーを掴む。
  move する前にポインタを取っておくのが確実
- 名前は `MakeUniqueName()` で一意にする。レンダラー名も同じ名前にするので、ここで潰しておくと下流が楽
- **モデル名は `ScanModelFolders()` の結果からしか選ばせない**。`ModelLoader` は存在しないパスで
  `ASSERT_MSG` に落ちるので、UIから任意文字列を渡してはいけない

**エディタのImGuiはフレームの先頭で走る**（`MyGameTitle::Update()` の `ImGuiManager::Begin()`）。
`Object3dManager::Update()` より前なので、`objects_` への追加は反復中に起きない。
削除も `DeleteObject()` が `isAlive=false` を立てるだけで、実際の erase は次フレーム頭の
`RemoveObjects()` なので安全。

**ラベルは ImGui の ID 元**。ラジオボタン `"モデル"` とコンボ `"モデル"` を同じポップアップに置いて
ID衝突を出した（1.92 は画面に警告を出してくれる）。コンボ側を `"使うモデル"` に変えて解消。

**回転はクォータニオンのまま扱う**。オイラー角に直して往復させると誤差が溜まるので、
Inspector は「回転を加算(deg)」の差分入力と「絶対指定」ボタンを分けている。

**`TextureManager::GetMetaData()` は `operator[]` で引くので副作用がある**。
未登録の名前を渡すと空の要素が生えてしまい、一覧に `0x0` の幽霊行が並ぶ。
一覧を舐める用途には `TryGetMetaData()`（find するだけ・見つからなければ nullptr）を足した。

**`ImGui::Image` に渡せるのはImGui自身のディスクリプタヒープの中だけ**。
`TextureManager` のSRVはエンジン側のシェーダ可視ヒープにあるので、そのままでは使えない。
シェーダ可視ヒープ同士は `CopyDescriptors` もできないので、
ImGuiのヒープに**1枠だけ**確保して、選択が変わるたびにリソースからSRVを作り直している。
枠を固定しておけば ImGui のヒープ(64枠)を食い潰さない。

**ポストエフェクトの並び替えは描画ループの外でやる**。一覧を描いている最中に
`effects_` を swap すると反復が壊れるので、「どれをどっちへ」だけ覚えて表を描き終えてから適用する。
`OffScreenManager` は `effects_` と `paths_` を並行して持っていて、実行順は `paths_` 側で決まるため、
両方を同時に入れ替えないとズレる。

---

### Phase 4 — エンジンランタイムから ImGui を剥がす ✅ 完了

| もとの場所 | 移動先 |
|---|---|
| `LightManager::DrawLightEditor` | `LightWindow.cpp`（Phase 3 で実施） |
| 各PostEffect の `Update()` 内の ImGui | `RenderWindow.cpp` のチェーン表で、選択したエフェクトのパラメータとして表示 |
| `CascadedShadowMap::DrawDebugUI` | `RenderWindow.cpp` の「シャドウ (CSM)」セクション |
| `TimeManager` / `DeltaTime` の `Update()` 内の ImGui | `Engine/Editor/Windows/TimeWindow.cpp` |
| `Graphics/Rendering/Particle/ParticleEditor.{h,cpp}` | `Engine/Editor/Windows/` へ移動（中身はそのまま） |

エンジン側に足したアクセサ:
`GaussianEffect` / `GrayEffect` / `SmoothEffect` の設定構造体を public にして `GetEffectData()`、
`CascadedShadowMap` に `GetShadowDistance() / GetShadowFar() / GetSplitLambda() / GetLights() / GetCamera()`。
`OutlineEffect` は isActive しか無かったので、チェーン表のチェックボックスに吸収して UI ごと削除。

**残した逆流（意図的）**:

| ファイル | 理由 |
|---|---|
| `MyGameTitle.cpp` | App と Engine の合流点。`Editor::SetContext` / `AppEditor::Register` を呼ぶ |
| `ImGuiManager.{h,cpp}` | ImGui の DX12 バックエンド。`Editor::Draw()` の駆動元 |
| `LightManager.cpp` / `CollisionManager.cpp` / `ForwardSceneRenderPass.cpp` | `EditorDebugDraw::IsEnabled()` の描画ゲート。UIではない |

**確認コマンド**:
`grep -rln "Editor/Core/" Engine --include=*.cpp --include=*.h | grep -v "Engine/Editor/"` が上の5ファイルだけになる。

---

### Phase 5 — App のエディタを分離する ✅ 完了

```
App/Editor/
  AppEditor.{h,cpp}            ← 登録の入口 + 編集対象の探索
  Windows/
    AppEditorWindows.h         ← 6つの Draw 関数の宣言
    PlayerWindow.cpp           ← Player の状態 + Attack Player
    AttackEditorWindow.cpp     ← Attack Editor / Attack Derivative Editor
    EnemyWindow.cpp            ← シーン内の敵を1つの表にまとめた
    CameraWorkWindow.cpp       ← GameCamera
    StylishWindow.cpp          ← スタイリッシュランク
    HitEffectWindow.cpp        ← HitEffect / HitPostEffect
```

**編集対象はシーンから渡してもらわない**。`AppEditor::FindPlayer()` /
`FindGameCamera()` が `Object3dManager` / `CameraManager` から `dynamic_cast` で毎フレーム引き直す。
シーンを跨いでも壊れず、対象が居ないシーンでは各ウィンドウが「いません」と出すだけで済む。
おかげで `GameScene` / `EditScene` の `DebugUpdate()` は空になり、シーン側の改修が要らなかった。

- `Object3dManager::FindObject()` は名前が要るうえ見つからないとログを吐くので使わない
- `CameraManager::FindCamera()` は `operator[]` なので、`GetCameraNames()` の結果だけを回すこと

**ゲームクラス側の始末**:

| クラス | 対応 |
|---|---|
| `Ground` / `Prop` / `Enemy` / `GruntMelee` | `DebugGui()` の中身が `Object3d::DebugGui()` だけだった。**Inspector が同じことをするので丸ごと削除** |
| `Player` / `PlayerStateMachine` / `BossKnight` | 表示項目を `PlayerWindow` / `EnemyWindow` へ移して削除 |
| `GameCamera` | パラメータは元々 GlobalVariables なので App/Editor から直接叩く。実行時状態だけ `MakeEditorStatus()` で渡す |
| `StylishScoreManager` | 同上。状態は `MakeEditorStatus()`、テストボタンは既存の public API |
| `HitEffectSystem` | `GetEditorGroupName()` / `MakeEditorKey()` / 接続状態のゲッターを足して完全移設 |
| `HitPostEffect` | `IsPlaying()` と各エフェクトのゲッターを足して完全移設 |
| `PlayerCombat` / `AttackPlayer` | **UIの中身はクラス側に残した**（下記） |

**PlayerCombat / AttackPlayer だけ本体を残した理由**:
攻撃グラフ（`attackGraph_` / `states_`）と再生状態はこのクラスの内部表現そのもので、
UIを外へ出すと private を20個近く公開することになる。デバッグ表示のためだけに
公開面を広げるのは割に合わないと判断した。代わりに
**ウィンドウの開閉と描画タイミングは App/Editor が握る**（`EditorWindow::Begin/End` はあちら側）。
`PlayerCombat::Update()` が自分でUIを呼ぶのはやめた。

**確認コマンド**:
`grep -rln "Editor/Core/" App --include=*.cpp --include=*.h | grep -v "^App/Editor/"` が
`PlayerStateAttack.cpp`（`EditorDebugDraw` の描画ゲート）だけになる。

---

### Phase 6 — 仕上げ ✅ 完了

**Windowメニューを「エンジン」「ゲーム」に分けた。**
カテゴリ（Camera / VFX など）はエンジンとゲームで共有したいので、分類とは別軸で
`EditorWindow::Origin` を持たせた。判定は自動で、`Begin()` の呼び出し側は何も意識しない:

- `Editor::Draw()` が drawer を呼ぶ直前に `EditorWindow::SetCurrentOrigin()` を立てる
- レジストリは登録の瞬間にそれを引いて `Entry::origin` に記録する
- `Editor::Initialize()` の中で登録された drawer が「エンジン」、それ以外が「ゲーム」
- 例外的に、`ImGuiManager` が持つゲームビューだけ `AddWindowDrawer(..., Origin::Engine)` と明示する

Ctrl+P のクイックオープンも「エンジン / Camera」のように出所を併記するようにした。

**ハマった点**: エンジンとゲームで同じカテゴリ名（"Camera" など）が並ぶと
`ImGui::BeginMenu` のIDが衝突して片方が丸ごと消える。セクションごとに `PushID` して解決。

**Phase 4 の積み残しも回収した**: `ParticleManager::Update()` の末尾が `editor_->Draw()` を
呼んでいたので、`Engine/Editor/Windows/ParticleEditorWindow.cpp` へ移した
（`ParticleManager` には `GetEditor()` を足しただけ）。空だった `ParticleManager::DebugGui()` も削除。

**書いたもの**: `docs/EditorArchitecture.md`（完成形の説明・拡張手順・引っかかりやすい点）、
`README.md`（ビルド手順・ディレクトリ・エディタの概要）。

---

## 4. 設計上の判断メモ

**なぜ `EditorContext` を挟むのか**
どうせシングルトンなので `GetInstance()` を直に呼んでも動く。ただしそれをやると
「エディタがどのサービスに触っているか」がgrepでしか分からなくなる。
`EngineContext` に集約しておけば、後でエディタを別exeやツールに切り出すときの境界がそのまま使える。

**なぜ `std::function` の登録制にするのか**
エンジンがAppを直接呼ぶわけにいかないため。仮想関数のインターフェース（`IEditorExtension`）でもよいが、
今回は「Appが数個のdrawerを足すだけ」なので `std::function` のほうが軽い。

**やらないこと**
- エディタの別exe化（今の規模では過剰）
- ImGuiのマルチビューポート有効化（別件。`ImGuiManager.cpp` にコメントで残っている）
- `Engine/Math/*.cpp` の `PrintOnImGui()` は生の `ImGui::Begin` のまま（使い捨てなのでレジストリを汚さない）

## 5. 着手順の推奨

Phase 0 → 1 → 2 まではほぼ機械作業で、ここまでやると「土管」が通る。
そこから **Phase 3 の Hierarchy + Inspector + Asset Browser** を先に作ると、
やりたかったこと（エンジン機能をエディタから呼ぶ）の成果が一番早く目に見える。
Phase 4 と 5 は分量が多いので、ウィンドウ単位で少しずつ移せばよい。
