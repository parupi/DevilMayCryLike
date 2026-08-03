# エディタの構成

GuchisEngine のエディタ（`_DEBUG` ビルドのみ）の全体像。
「どこに何があるか」「新しいウィンドウをどう足すか」をここにまとめる。
どういう経緯でこの形になったかは `EditorArchitecture_Plan.md` を参照。

---

## 1. 依存の向き

```
    App/Editor  ──────▶  Engine/Editor  ──────▶  Engine ランタイム
        │                                            ▲
        └────────────────────────────────────────────┘
                    App ランタイム
```

守るべきルールは3つ。

1. **`Engine/Editor` は App を include しない**。App のウィンドウ名も持たない
2. **エンジンのランタイムは `Engine/Editor` を include しない**（下の例外を除く）
3. 合流点は `MyGameTitle::Initialize()` だけ。App と Engine の両方を知っている唯一の場所

### 例外（意図的に残している逆流）

| ファイル | 理由 |
|---|---|
| `Engine/Application/MyGameTitle.cpp` | 合流点。`Editor::SetContext()` と `AppEditor::Register()` を呼ぶ |
| `Engine/Debugger/ImGuiManager.{h,cpp}` | ImGui の DX12 バックエンド。`Editor::Draw()` の駆動元 |
| `Engine/World3D/Light/LightManager.cpp` | `EditorDebugDraw::IsEnabled(LightGizmo)` の描画ゲート |
| `Engine/World3D/Collider/CollisionManager.cpp` | 同上（Collider） |
| `Engine/Graphics/.../ForwardSceneRenderPass.cpp` | 同上（Grid） |
| `App/.../PlayerStateAttack.cpp` | 同上（AttackTrail） |

いずれも「描くかどうかのフラグを読む」だけで、UIは持たない。

**確認コマンド**:

```bash
grep -rln "Editor/Core/" Engine App --include=*.cpp --include=*.h | grep -v "/Editor/"
```

これが上の6ファイルだけになっていれば健全。

---

## 2. ファイル配置

```
Engine/Debugger/
  ImGuiManager.{h,cpp}      ImGuiのDX12バックエンド・SRVヒープ・ゲームビュー用RT
  GlobalVariables.{h,cpp}   JSONパラメータ（エディタ専用ではない）
  LeakChecker.{h,cpp}

Engine/Editor/Core/
  EditorHost.{h,cpp}        拡張ポイントと駆動の集約。外から使うのはこの1枚
  EditorContext.{h,cpp}     Editor::Ctx() で EngineContext を引く
  EditorWindowRegistry.*    自己登録式のウィンドウレジストリ。Windowメニュー / Ctrl+P
  EditorMenuBar.*           File/Scene/Window/Debug Draw/Layout/Help + 再生コントロール
  EditorLayout.*            DockBuilder のレイアウトプリセット（登録制）
  EditorDebugDraw.*         コライダー/ライト/攻撃軌跡/グリッドの表示トグル
  EditorStats.*             FPS・VRAM・RAM
  EditorGameView.*          ゲーム画面を1280x720のRTへ描いて ImGui::Image で出す
  EditorViewMath.*          ゲームビューに重ねるものの投影とレイ（ギズモと選択で共有）
  EditorGizmo.*             ゲームビュー上の移動／回転／拡縮ギズモ
  EditorPicking.*           ゲームビューの絵をクリックして選ぶ
  EditorSelection.*         選択中の Object3d（名前で保持）
  EditorAssetUtil.*         モデルフォルダ走査・モデル名の逆引き

Engine/Editor/Windows/      エンジン機能のウィンドウ
  HierarchyWindow    Object3d 一覧・生成・複製・削除
  InspectorWindow    選択中の Transform / 描画設定 / Renderer / Collider
  AssetBrowserWindow モデル・テクスチャの一覧とプレビュー
  LightWindow        ライトの一覧・追加・編集・削除
  RenderWindow       ポストエフェクトのチェーン / CSM / スカイボックス
  CameraWindow       カメラ一覧・アクティブ切替・パラメータ
  AudioWindow        サウンドの走査・読み込み・試聴
  TimeWindow         DeltaTime / TimeManager
  ProfilerWindow     各マネージャの保持数・SRV使用率
  DebugLogWindow     Logger の内容
  ParticleEditor.*   パーティクル／VFX（4ウィンドウ）
  ParticleEditorWindow.*  上を回すだけの薄いラッパ

App/Editor/
  AppEditor.{h,cpp}         登録の入口 + 編集対象の探索
  Windows/
    AppEditorWindows.h      6本の宣言
    PlayerWindow            プレイヤーの状態 + 攻撃プレビュー
    AttackEditorWindow      Attack Editor / Attack Derivative Editor
    EnemyWindow             シーン内の敵の一覧
    CameraWorkWindow        GameCamera のカメラワーク調整
    StylishWindow           スタイリッシュランク
    HitEffectWindow         HitEffect / HitPostEffect
```

---

## 3. 新しいウィンドウを足す

### エンジンの機能を出す場合

1. `Engine/Editor/Windows/XxxWindow.{h,cpp}` を作る
2. 中身を書く。エンジンのサービスは `Editor::Ctx()` から引く

```cpp
#include "XxxWindow.h"
#ifdef _DEBUG
#include "Editor/Core/EditorHost.h"   // Ctx() と EditorWindow がこれ1枚で入る
#include <imgui/imgui.h>

void Editor::DrawXxxWindow()
{
    if (!EditorWindow::Begin("Xxx", EditorWindow::Category::kEngine)) {
        return;
    }
    // Ctx() のポインタは SetContext 前だと全部 nullptr。必ず確認する
    if (auto* objects = Ctx().object3dManager) {
        ImGui::Text("%zu 個", objects->GetAllObject().size());
    }
    EditorWindow::End();
}
#endif
```

3. `Editor::Initialize()`（`EditorHost.cpp`）の `g_registeringEngineDefaults` のスコープ内へ
   `AddWindowDrawer([] { DrawXxxWindow(); });` を足す
4. `premake5` を再実行（`premake5.lua` はグロブ収集なので追記は不要）

### ゲーム固有の機能を出す場合

1. `App/Editor/Windows/XxxWindow.cpp` を作り、`AppEditorWindows.h` に宣言を足す
2. 編集対象は `AppEditor::FindPlayer()` などで引く（シーンから渡してもらわない）
3. `AppEditor::Register()` に `Editor::AddWindowDrawer([] { DrawXxxWindow(); });` を足す

`AppEditor::Register()` から登録した drawer が開いたウィンドウは、
Windowメニューの「ゲーム」側へ自動的に並ぶ。呼ぶ側が意識することは何も無い。

### 共通の約束

- **`ImGui::Begin/End` は直接使わない**。`EditorWindow::Begin/End` を使うとメニューにも保存対象にも自動で載る
- `Begin()` は非表示・折りたたみのどちらでも `false`。false のとき `End()` を呼ばないこと
- ウィンドウは**初めて描かれたときに遅延登録**される。一覧を手で管理する必要はない
- カテゴリはただの文字列。`Category::` の定数以外も渡してよい

---

## 4. 押さえておく仕組み

### 駆動

`ImGuiManager::Begin()` が `Editor::Draw()` を1回呼ぶだけ。その中で

1. `EditorWindow::NewFrame()` / `EditorStats::Update()`
2. `DockSpaceOverViewport()` → `EditorLayout::ApplyPendingPreset()`
3. `EditorMenuBar::Draw()`
4. 登録された drawer を順に呼ぶ（呼ぶ前に `SetCurrentOrigin()` で出所を立てる）

エディタは `MyGameTitle::Update()` の**先頭**で走る。`Object3dManager::Update()` より前なので、
エディタからのオブジェクト追加・削除が反復中に起きることはない。

### 保存

| ファイル | 内容 | 書き出し |
|---|---|---|
| `Resource/GlobalVariables/Editor/EditorWindows.json` | ウィンドウの表示状態 | `Editor::Finalize()` / Windowメニュー |
| `Resource/GlobalVariables/Editor/EditorDebugDraw.json` | デバッグ描画のトグル | 同上 |
| `Resource/GlobalVariables/Editor/EditorGizmo.json` | ギズモの操作モード・スナップ設定 | 同上 |
| `Resource/GlobalVariables/Editor/EditorPicking.json` | クリック選択・選択枠のトグル | 同上 |
| `imgui.ini` | ドッキング配置・ウィンドウサイズ | ImGui が自動 / Layoutメニュー |
| `Resource/GlobalVariables/<各グループ>/` | ゲーム・エンジンの調整値 | Ctrl+S / 各ウィンドウの Save |

ウィンドウは遅延登録なので、`LoadSettings()` の時点では値を `g_saved` に貯めておき、
登録の瞬間に引く仕掛けになっている。

**配置が崩れたら** Layoutメニューの「配置をリセット」、
それでも直らなければ `imgui.ini` を消せば既定に戻る。

### レイアウトプリセット

登録制。エンジンが 標準 / アセット / 描画・ライト / VFX作業、
App が バトル調整 / カメラ調整 / VFX作業(ゲーム) を登録する。

`DockBuilder` は DockSpace を作った直後の同フレームでないと正しく分割できないので、
メニューからは `RequestPreset()` で予約して次フレームに適用する。

### ゲームビューに重ねるもの

`EditorGameView::DrawWindow()` の `ImGui::Image` の直後に、この順で呼ぶ。

```cpp
const ImVec2 imagePos = ImGui::GetItemRectMin();  // 画像の実際の左上
EditorGizmo::DrawOverlay(imagePos, imageSize);
EditorPicking::HandleGameView(imagePos, imageSize);  // ギズモの後
```

投影とレイは `EditorViewMath`（`EditorView::Build/WorldToScreen/ScreenToRay`）に集めてある。
アクティブカメラと画像の矩形から `EditorView::Context` を作り、両者が同じ前提を共有する。

**順番が意味を持つ**。`EditorPicking` は `EditorGizmo::IsOver()` を見て、
ギズモを掴んだクリックを横取りしないようにしている。逆順にすると、
ギズモの矢印をクリックした瞬間に後ろのオブジェクトへ選択が飛ぶ。

### ギズモ

Hierarchy / Inspector と同じ選択（`EditorSelection`）を、Game ウィンドウの絵の上で直接動かす。
ImGuizmo などの外部ライブラリは使わず、エンジンの `Matrix4x4` / `Quaternion`
（**行ベクトル・左手系**、`v * M`、平行移動は `m[3][*]`）に合わせて `EditorGizmo.cpp` が全部持っている。

呼び出しは `EditorGameView::DrawWindow()` の `ImGui::Image` 直後の1箇所だけ。
位置合わせに画像の実際の左上（`ImGui::GetItemRectMin()`）が要るので、必ずここで呼ぶ。

- **編集するのは `WorldTransform` のローカル値**。親がいる場合はワールドでの操作量を
  `TransformNormal(delta, Inverse(parentWorld))` で親のローカル空間へ落としてから書き戻すので、
  子オブジェクト（キャラの武器など）でも見たとおりに動く
- **表示に使う行列は毎フレーム組み直す**。`matWorld_` はゲーム側の更新でしか動かないため、
  そのまま読むとポーズ中に追従しなくなる
- **ドラッグ中は軸・原点・掴んだ位置をすべて開始時のもので固定する**。毎フレーム引き直すと
  動かした結果が次の計算に混ざって暴走する
- **拡縮は常にローカル軸**。ワールド/ローカルの切り替えは移動・回転にだけ効く
- 大きさは画面上で一定。カメラ右方向に1m離れた点を投影して「1mが何ピクセルか」を測り、
  そこから逆算している（透視でも正射影でも同じ式で足りる）

| キー | 動作 |
|---|---|
| Ctrl+1 / 2 / 3 | 移動 / 回転 / 拡縮 |
| Ctrl+L | ワールド軸 ⇔ ローカル軸 |
| Ctrl+G | ギズモの表示切替 |
| ドラッグ中の Ctrl | スナップの有無を一時的に反転 |

W/E/R も使えるが、ゲームの移動入力と衝突するので既定はオフ（Gizmoメニューで有効化）。

### クリックで選択

ゲームビューの絵をクリックすると、その下にある `Object3d` が `EditorSelection` に入る。
Hierarchy / Inspector / ギズモはそこを見ているので、そのまま追従する。

- **判定はモデルのCPU側の頂点**。モデルごとに1度だけ三角形リストとローカルAABBに焼いて
  `BaseModel*` をキーにキャッシュする（`Editor::Finalize()` で捨てる。
  `ModelManager` はモデルを個別に解放しないのでキーは死なない）
- **レイはモデルのローカル空間へ持っていく**。このとき向きを**正規化しない**のがコツで、
  出てくる t がそのままワールドでの距離になり、スケールの違うオブジェクトどうしで前後を比べられる
- **スキンモデルはAABBだけ**。CPU頂点がバインドポーズのままで、アニメ中の三角形は当てにならない
- `GetIsDraw()` が false のオブジェクトは選べない。見えていないものを掴んでも混乱するだけ
- **同じ場所を続けてクリックすると、重なった奥のオブジェクトへ順に送る**
  （4px 以内なら同じ場所とみなす）。何も無いところをクリックすると選択解除
- 選択中は本体に沿った箱（ローカルAABBの8隅をワールドへ運んだもの）で囲う

重いのはクリックした瞬間だけで、毎フレーム走るのは選択枠の描画（キャッシュ引き）のみ。

### ショートカット

| キー | 動作 |
|---|---|
| Ctrl+P | ウィンドウを検索して開く |
| Ctrl+S | 全パラメータを保存 |
| Ctrl+R | シーンをリロード |
| F5 | 再生 / 一時停止 |
| F10 | コマ送り（一時停止中） |

ギズモのショートカットは上の「ギズモ」を参照。

再生コントロールは `DeltaTime`（`SetPaused` / `RequestStep` / `SetDebugTimeScale`）に入っている。
ここが唯一の絞り口で `TimeManager` を含む下流全部に効くので、ゲーム側は無改造。
ポーズ中も進む実時間が要るときは `DeltaTime::GetUnscaledDeltaTime()`。

---

## 5. 引っかかりやすい点

- **`Object3d::DebugGui()` を Inspector から呼ばない**。派生クラスが override して自分のウィンドウを
  開くので入れ子になる。Inspector はゲッター経由で自前に描いている
- **選択はポインタではなく名前で持つ**。`RemoveDeadObject()` で実体が消えるため
- **`Object3dManager::FindObject()` / `CameraManager::FindCamera()` を毎フレーム呼ばない**。
  前者は見つからないとログを吐き、後者は `operator[]` で空要素を生やす
- **`TextureManager::GetMetaData()` にも同じ副作用がある**。一覧を舐めるなら `TryGetMetaData()`
- **`Model::GetModelData()` / `SkinnedModel::GetModelData()` は値を返す**（メッシュごと丸コピー）。
  毎フレーム呼ぶと確実に落ちる。EditorPicking は焼くときの1回だけ呼んでいる
- **`MathUtils` の `Transform()` は w=0 で assert する**。潰れた行列を通す可能性がある場所では
  自前で座標変換すること（EditorPicking の `TransformPoint`）
- **`ImGui::Image` に渡せるのは ImGui 自身のディスクリプタヒープの中だけ**。
  エンジンのSRVはシェーダ可視ヒープにあり `CopyDescriptors` もできないので、
  Asset Browser は ImGui のヒープに1枠だけ確保してSRVを作り直している
- **ImGui のラベルはID元**。同じウィンドウ内でラジオとコンボに同じ文字列を使うと衝突する
  （1.92 は画面に警告を出してくれる）。ラベルが状態で変わるボタンは `##固定ID` を付ける
- **`PushStyleColor` の判定はウィジェットを出す前に確定させる**。
  `if (flag) Push; if (Button(...)) flag = !flag; if (flag) Pop;` は Push と Pop の数がずれて
  `"Calling PopStyleColor() too many times!"` で落ちる
- **`SetCursorPos`/`SetCursorScreenPos` で動かしたら、その後にアイテムを1つ出す**。
  何も出さずに `End()` すると「境界を広げる意図か」と assert する。
  ゲームビューに重ねたギズモのツールバーは `ImGui::Dummy(ImVec2(0,0))` で締めている
- **`u8"..."` を使わない**。C++20 では `const char8_t*` になって `const char*` に渡せない。
  素の `"日本語"` でよい（`/utf-8` でビルドしている）
