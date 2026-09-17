# ステージ編集をエンジン内で完結させるために足りないもの

Blender アドオン (`Externals/level_editor/`) で作っている `Resource/Stage/Stage.json` を、
ビルド中のエディタ（`Engine/Editor/` + `App/Editor/`）でのオブジェクト生成・配置だけで
作れるようにするための不足一覧。

> **進捗（2026-08-04）**: Blender との併用はしない方針が決まり、**Step 1〜4 がすべて完了**。
> Blender アドオン (`Externals/level_editor/`) は削除済み。
> ステージはエンジン内エディタだけで作れる。残課題は下の「残っているもの」を参照。
> ステージデータはエンジン空間の新フォーマット（`GuchisStage` v2）に作り直し、
> `Resource/Stage/*.json` 5本を変換済み。Blender アドオンが吐く旧フォーマットはもう読めない
> （`SceneLoader::Load` が format/version を見てはっきり弾く）。
> 各項目の頭に **[済]** / **[未]** を付けてある。

---

## 0. 現状の対応表

| Blender でできること | エンジン内エディタの現状 |
|---|---|
| Player / TutorialDummy / GruntMelee / BossKnight / Ground / Prop / PointLight / Event×4 の生成 | **素の `Object3d` しか作れない**（Hierarchy「+作成」/ AssetBrowser「配置」） |
| `class_name` の指定 | **概念が無い**（`Object3d` は `name_` しか持たない） |
| `file_name`（使用モデル）の指定 | 生成時のみ指定可。後から差し替えは「読み込み済みモデル」からのみ |
| コライダーの追加・type・center・size の編集＋ビューポート表示 | **Inspector は読み取り専用**（名前と形状を出すだけ） |
| PointLight の色・明るさ・距離・減衰 | **生成も編集もできない** |
| Prop の付属ライト（ランタン等） | **同上** |
| イベントの生成、出現させる敵の指定 | **生成できない** |
| 親子構造（children の再帰出力） | Hierarchy はフラット。**そもそも `SceneLoader` が children を読んでいない** |
| `disabled`（出力しない） | 相当するものが無い（`SetIsDraw` は描画だけ切る別物） |
| Ctrl+Z / 複数選択 / 整列 / スナップ | Undo なし、単一選択のみ、ギズモのスナップのみ |
| **JSON への書き出し** | **保存機能が一切無い** |

エンジン内エディタが既に持っているもの（作り直し不要）:
移動/回転/拡縮ギズモ、ゲームビューのクリック選択、F9 デバッグカメラ、Hierarchy の生成/複製/削除、
Transform 編集、コライダー/ライトのデバッグ描画、AssetBrowser のモデル一覧。

---

## 1. 決定的に足りないもの（これが無いと何をしても残らない）

### A. シーンの保存（最優先）

- `App/Stage/SceneLoader.cpp` は **読み込み専用**。書き出す側が存在しない。
- `EditorMenuBar::DrawFileMenu()`（`Engine/Editor/Core/EditorMenuBar.cpp:38`）にあるのは
  GlobalVariables の保存とエディタ設定の保存だけ。「シーンを保存」「名前を付けて保存」「新規シーン」が無い。
- `Ctrl+S` は既に `GlobalVariables::SaveAllFiles()` に割り当て済み。シーン保存とどちらを割り当てるか整理が要る。

必要なもの: `SceneSaver`（`std::vector<SceneObject>` → JSON）と、
**ランタイムのオブジェクト群 → `SceneObject` へ戻す逆変換**。後者が B / C の話につながる。

### B. ランタイムのオブジェクトが「自分が何者か」を持っていない

`Object3d`（`Engine/World3D/Object/Object3d.h`）が持っているのは `name_` / transform / renderers /
colliders / DrawOption だけ。保存しようにも書き出す情報が無い。

足りない情報:

| 保存に要る項目 | 現状 |
|---|---|
| `class`（Ground / Prop / PointLight / GruntMelee / Event_*） | **どこにも保持していない** |
| `file_name`（使用モデル名） | `Ground::modelName_` / `Prop::modelName_` は private・getter 無し。`Editor::FindLoadedModelName()` でモデル実体から逆引きはできるが、`Ground` が持つ論理的な file_name とは別物 |
| ライト設定 | `StagePointLight` / `Prop` の private メンバ。getter 無し |
| イベント設定（対象の敵名など） | `EnemySpawnEvent::enemies_` は `Enemy*` の配列。名前へ戻す口が無い |

方針の候補:
1. `Object3d` に `className_` を持たせ、`virtual void Serialize(nlohmann::json&) const` を生やす
2. あるいはエディタ側に「オブジェクト → SceneObject」の変換器を置き、
   `dynamic_cast` で分岐する（`SceneBuilder` の逆。App 側に置くことになる）

`SceneBuilder::BuildObject()` が `dynamic_cast<Ground*>` などで分岐しているので、2 のほうが既存の形に近い。

### C. クラスを指定したオブジェクト生成

- `HierarchyWindow.cpp:103` の `SpawnObject()` は `std::make_unique<Object3d>` 固定。
- `Object3dFactory`（`App/Scene/Object3dFactory.h`）は登録制になっているが、
  **登録済みクラス名を列挙する API が無い**（`GetClassNames()` 相当）。
- さらに `Engine/Editor` は App を include しない規約（`docs/EditorArchitecture.md`）。
  `AbstractSceneFactory` / `GetSceneNames()` と同じ形で、Engine 側にインタフェースを置いて
  App が実装する必要がある。

---

## 2. 機能として足りないもの

### D. コライダーの追加・削除・編集

- `InspectorWindow::DrawColliderSection()`（`Engine/Editor/Windows/InspectorWindow.cpp:138`）は
  `BulletText` で名前と形状を出すだけの**読み取り専用**。
- Blender 側は type(BOX/SPHERE) / center / size を編集でき、水色ワイヤーで常時表示（`draw/draw_collider.py`）。
- エンジン側は `CollisionManager::AddCollider()` が public なので追加自体は可能。要るのは UI と、
  `SceneBuilder::ApplyCollider()` と同じ変換（BOX → `OBBCollider` 化、Y/Z 入れ替え）を通す口。
- コライダーの center/size をビュー上でドラッグするギズモも無い（`EditorGizmo` はオブジェクトの transform 専用）。
- `Ground` は `Initialize()` で `GetCollider(name_)->category_ = Ground` を触るので、
  **コライダーが無い状態で `Ground` を生成すると nullptr 参照になる**。生成順の設計が要る。

### E. イベントの生成と参照の設定 **[済 — Step 3]**

- `EventFactory::Create()`（`App/GameObject/Event/EventFactory.cpp:7`）は if-else のハードコード。
  登録制ですらないので、エディタから種類を列挙できない。
- 生成できたとしても `EventManager::AddEvent()` への接続と、
  「出現させる敵」をシーン内のオブジェクトから選ぶ UI（Blender の `PointerProperty` 相当）が要る。
- 参照は**名前で持つ**べき（`EditorSelection` と同じ理由。`Enemy*` を握ると削除で dangling する）。

**ただし、実装すべき範囲は Blender より狭い。** 現状 C++ 側が読んでいない／使っていない項目がある:

| Blender で設定できる | C++ の実装 |
|---|---|
| EnemySpawn の `trigger`（AREA/TIME/MANUAL） | `SceneLoader` は読むが `EnemySpawnEvent` に反映先が無い（**死にデータ**） |
| EnemySpawn の `delay`（敵ごとの遅延秒） | 同上（`EnemySpawnEvent` に遅延の仕組みが無い） |
| Clear の `ALL_ENEMIES_DEFEATED` / `REACH_OBJECT` / `TIMER` | `SceneLoader` は `DEFEAT_ENEMIES` の `targets` しか読まない。`ClearEvent` も敵リストのみ |
| Clear の `time` / `target` | `EventCondition` に該当フィールドが無い |

→ エディタ側で作るのは **EnemySpawn(敵リスト) / ForceBattle(敵リスト) / BossSpawn(ボス1体) /
Clear(撃破対象の敵リスト)** の4つで足りる。上表の項目は「実装するか捨てるか」を決める。

### F. ライト（PointLight / Prop の付属ライト）

- `StagePointLight::SetLight()` / `Prop::SetLight()` はどちらも
  **「`Initialize()` より前に呼ぶこと」**という制約付き。実行中に変更できない。
- Light ウィンドウ（`Engine/Editor/Windows/LightWindow.cpp`）が触るのは `LightManager` が持つ
  `DynamicPointLight`。これは `StagePointLight` が生成した実体であって、
  **保存対象である `StagePointLight` 側のパラメータとは繋がっていない**。
  Light ウィンドウで色を変えても保存には乗らない。
- 要るもの: `SetLight()` の実行時反映（`light_` へ即時書き戻し）、getter、Inspector からの編集 UI。

### G. モデルの差し替え（file_name）

- `ModelRenderer::SetModel()` はある。`Model::DebugGui()` の "Models" コンボから呼べる。
- ただしそのコンボは **`ModelManager::models`（＝読み込み済み）だけ**を列挙する。
  AssetBrowser が使う `Editor::ScanModelFolders()`（ディスク上の全モデル）とは別物で、
  未読み込みのモデルには差し替えられない。
- `Ground` / `Prop` の `modelName_` は更新されないので、差し替えても保存には乗らない（B の話）。

### H. 親子構造（children）

- Blender の export は `children` を再帰出力する（`export_scene.py:145`）。
- **`SceneLoader::ParseObject()` は `children` を読んでいない**。
  つまり現行パイプラインは親子を既に捨てている（Blender 側だけの機能）。
- `WorldTransform::SetParent()` / `GetParent()` はあるので、揃えるならエンジン側の対応から。
- Hierarchy はフラットなリスト。ツリー表示・ドラッグ&ドロップでの親子付けが無い。
- → **「親子は使わない」と決めれば作業量が減る**。使うなら Loader / Builder / Hierarchy の3箇所に手が要る。

### I. `disabled` に相当するフラグ

Blender の `disabled` は「JSON に出力しない」。エンジンの `SetIsDraw(false)` は描画だけ切る別物で、
コライダーもロジックも生きたまま。保存対象にする場合は別フラグが要る。

---

## 3. 作業性として足りないもの（無いと実用にならない）

### J. Undo / Redo

Blender は Ctrl+Z 前提。手作業で数十個のオブジェクトを並べる以上、これは実質必須。
`EditorSelection` と同様に、コマンドを名前ベースで積む形にしないと削除で壊れる。

### K. 複数選択・整列・配列複製

- `EditorSelection` は**単一選択のみ**（`std::string` 1本）。
- モジュラーな壁（`ModularStoneWall` など）を並べる用途だと、まとめて移動・等間隔配置・
  ミラー・配列複製が効く。現状はギズモのスナップ（15.0）だけ。

### L. 複製が不完全

`DuplicateObject()`（`HierarchyWindow.cpp:240`）が引き継ぐのは
「最初のレンダラーのモデル」「transform」「DrawOption」だけ。
コライダー・クラス・ライト設定は落ちる。B が入れば自然に直せる。

### M. 編集モードと再生モードの分離

- 再生コントロール（F5 / コマ送り）はあるが、**「編集中」と「プレイ中」の区別が無い**。
- ゲームが動けば敵は移動し、プレイヤー位置も変わり、`ForceBattle` は敵を `SetActive(false)` にする。
  その状態で保存すると壊れたステージが出来上がる。
- **実測（2026-08-04）**: 再生したまま数分放置して保存したところ、
  プレイヤーは接地位置へ落ち（想定内）、**`TutorialDummy` の x が 6051 → 20959 と増え続けていた**。
  未起動（`isActive_ == false`）の敵は `Enemy::Update` が即 return するので自分では動かないが、
  コライダーは生きたままなので押し出しの経路（`Enemy.cpp` の `GetTranslation() += result.normal * result.depth`）
  で毎フレーム押され続けている。**これは Step1 以前からあるゲーム側のバグ**。
- Step 4 で**再生中の保存を禁止した**ので、これがステージデータを壊すことは無くなった。
  ただしドリフト自体は残っているので、押し出しを未起動の敵に効かせない直し方は別途必要。
- Blender は当然「編集専用」だった。エンジン内でやるなら、
  再生前の状態を退避する / 編集専用シーンを用意する / 保存時に初期値を書く、のいずれかが要る。
- `App/Scene/EditScene.cpp` は存在するが、現状は Stage.json を読まずに手書きでオブジェクトを置く
  古い実験用シーン。ステージ編集用に作り直すなら候補。

---

## 4. データ・パイプラインの穴

### N. 座標系をどちらに寄せるか **[済 — エンジン空間に統一した]**

`SceneBuilder::ApplyTransform()`（`App/Stage/SceneBuilder.cpp:29`）は
Blender(Z-up 右手) → エンジン(Y-up 左手) の変換をしている:

```
translate: y と z を入れ替え
rotate:    y と z を入れ替えたうえで全軸の符号を反転（鏡映変換なので）
scale:     y と z を入れ替え
collider:  offsetMin/Max も y/z 入れ替え
```

**案2（エンジン空間の新フォーマット）を採用し、上記の変換は撤廃した。**
既存の `Resource/Stage/*.json` は、同じ式を写した使い捨てのコンバータで一度だけ変換済み。
`SceneBuilder::ApplyTransform` は値をそのまま流し込むだけになっている。

**Blender アドオン (`Externals/level_editor/`) は削除した**（2026-08-04）。
旧フォーマットしか吐けず、残しておくと事故のもとになるため。
必要になったら git 履歴から戻せる。

### O. FBX が読めない

- **同梱の assimp は FBX を読めない**（検証済み: `No suitable reader found for the file format`）。
- Blender アドオンはこれを回避するため、`.fbx` しか無いモデルを **自動で `.obj` へ書き出し**、
  さらに mtl から実体の無いテクスチャ参照を掃除している
  （`menus/topbar_menu.py` の `ensure_game_loadable_model` / `_sanitize_mtl`）。
  エンジン内配置ではこの前処理が走らない。
- 加えて `Editor::ScanModelFolders()`（`Engine/Editor/Core/EditorAssetUtil.cpp:27`）は
  **`.fbx` も候補に出す**ので、fbx のみのモデルを選ぶと `ModelLoader` の `ASSERT_MSG` で落ちる。
  現状 `Resource/Models` に fbx のみのフォルダは無いので顕在化していないが、潜在的な地雷。
- 対応: 一覧から `.fbx` のみのフォルダを外す（最低限）／assimp を FBX 対応でビルドし直す／
  変換はこれまで通り Blender 側でやると割り切る。

### P. スキンモデルの扱い

`Editor::CreateModelObject()` は常に `ModelRenderer`（静的モデル）を付ける。
`Resource/Models` には glTF のスキンモデル（`Warrior` / `simpleSkin` / `walk` / `sneakWalk` / `BrainStem`）
が混ざっているので、これらを配置したときの挙動を決めておく必要がある。
なお Blender 側もキャラクターは `class_name` で置いており、モデルは C++ クラス側で決まる。

---

## 5. 実装順

**Step 1 — 保存できる形にする（完了 2026-08-04）**

| やったこと | 場所 |
|---|---|
| ステージデータをエンジン空間の新フォーマットへ作り直し（座標変換の廃止） | `App/Stage/SceneLoader.{h,cpp}` / `SceneBuilder.cpp` |
| `Object3d` に `className_` / `isStageObject_` / `SetModelName`・`GetModelName` を追加 | `Engine/World3D/Object/Object3d.h` |
| `Object3dFactory` を Engine へ移し、クラス名列挙と「モデル名を使うか」を追加 | `Engine/Scene/Object3dFactory.{h,cpp}` |
| Hierarchy の「+作成」でクラス・モデル・コライダーを選べるように | `Engine/Editor/Windows/HierarchyWindow.cpp` |
| `SceneSaver`（現在シーン → ステージデータ）と Stage メニュー | `App/Stage/SceneSaver.{h,cpp}` / `App/Editor/AppEditor.cpp` |
| `Resource/Stage/*.json` 5本を新フォーマットへ変換 | Stage / Title / Test / Untitled / stage1 |

新フォーマット（`format: "GuchisStage"`, `version: 2`）:

```json
{
  "name": "Ground", "class": "Ground",
  "translate": [x, y, z],
  "rotate": [x, y, z, w],          // クォータニオン（往復で誤差が出ないため）
  "scale": [x, y, z],
  "model": "Wall",                  // 旧 file_name。Ground / Prop だけが使う
  "collider": { "shape": "OBB", "offset": [...], "halfExtents": [...] },
  "light":  { "color": [...], "offset": [...], "intensity": f, "radius": f, "decay": f },
  "event":  { "type": "ForceBattle", "targets": ["GruntMelee3", "GruntMelee4"] }
}
```

**捨てたもの**（Blenderで設定できたが C++ が使っていなかった）:
`children` / `disabled` / EnemySpawn の `trigger` と `delay` /
Clear の `ALL_ENEMIES_DEFEATED`・`REACH_OBJECT`・`TIMER` 条件（`DEFEAT_ENEMIES` のみ残す）/
モデル名を使わないクラス（Player・敵）に付いていた `file_name`。

**保存の線引き**: `Object3d::IsStageObject()` が true のものだけ保存する。
`SceneBuilder` とエディタの生成経路だけがこのフラグを立てるので、
敵の武器のように実行中に生えるオブジェクトは巻き込まれない
（Hierarchy に `(保存対象外)` と出る）。

**Step 2 — Blender に無いと困る機能を埋める（完了 2026-08-04）**

| やったこと | 場所 |
|---|---|
| Inspector でコライダーを追加・編集・削除（OBB / 球。AABB は編集のみ） | `Engine/Editor/Windows/InspectorWindow.cpp` |
| モデル名を `Object3d` 本体へ集約し、Inspector から差し替え可能に | `Engine/World3D/Object/Object3d.h` / `InspectorWindow.cpp` |
| PointLight / Prop ライトの実行時反映（`ApplyLightParams`）と編集 UI | `App/GameObject/{Light/StagePointLight,Prop/Prop}.h` / `App/Editor/AppEditor.cpp` |
| Inspector の拡張ポイント `Editor::AddInspectorSection` | `Engine/Editor/Core/EditorHost.{h,cpp}` |
| モデル一覧のキャッシュを共有（`Editor::CachedModelFolders`） | `Engine/Editor/Core/EditorAssetUtil.{h,cpp}` |

- **モデル名は `Object3d::modelName_` に一本化した**。以前は `Ground` / `Prop` が各自 private に持っていて、
  素の `Object3d` にモデルを貼っても保存に出なかった（読み直すと消えていた）。
  `SceneBuilder` は「Initialize 後にレンダラーが無く、モデル名がある」オブジェクトへ `ModelRenderer` を貼る
- **Inspector からの差し替えは 3点セット**: `ModelManager::LoadModel`（`FindModel` は読み込み済みしか返さない）→
  `Object3d::SetModelName`（保存用）→ 各 `ModelRenderer::SetModel`（見た目）
- ライトは `SetLight()` / `ApplyLightParams()` が実体の `DynamicPointLight` へ即反映する。
  位置だけは `Update()` がトランスフォームから毎フレーム入れる（`Initialize` 時点では `matWorld_` がまだ組まれていない）
- **エンジンの Inspector は App の型を知らない**ので、ライトの UI は `AddInspectorSection` で App から差し込む。
  Step 3 のイベント設定も同じ口を使う

**残っているもの（Step 2 の範囲外にした）**
- コライダーのギズモ（オフセット・サイズをビュー上でドラッグ）。数値入力＋デバッグ描画で代用中
- ステージデータは1オブジェクト1コライダー。2つ目以降は保存されない（UI に明記してある）
- プリミティブ（Plane / Ring / Cylinder）で作った `Object3d` は保存できない。
  モデル名を持たないため、読み直すと空のオブジェクトになる

**Step 3 — イベント（完了 2026-08-04）**

| やったこと | 場所 |
|---|---|
| **`EventFactory` を廃止し、`Object3dFactory` に統合** | `App/GameObjectRegister.cpp` / `App/Stage/SceneBuilder.cpp` |
| `BaseEvent` が EventManager へ自己登録／解除 | `App/GameObject/Event/BaseEvent.cpp` / `EventManager.{h,cpp}` |
| Inspector でイベントの対象（敵）を名前で追加・削除 | `App/Editor/AppEditor.cpp` |

- **イベントは `BaseEvent : Object3d` なので、専用ファクトリを持つ理由が無かった**。
  `Object3dFactory` に登録すれば Hierarchy の「+作成」に自動で並び、
  `SceneBuilder::BuildEvent` は `dynamic_cast<BaseEvent*>` するだけで済む。
  `EventFactory.{h,cpp}` は削除した（if-else のハードコードで列挙もできなかった）
- **EventManager への登録は `BaseEvent` のコンストラクタが行う**。
  以前は `SceneBuilder` が明示的に呼んでいたので、エディタで作ったイベントは登録されず動かなかった。
  デストラクタで `RemoveEvent` もするので、エディタで削除してもぶら下がりが残らない
- Inspector の対象リストは**シーンにいる `Enemy` からしか選ばせない**。
  編集するのは `BaseEvent::targetNames_`（＝ステージに書かれる値）で、
  **実行中のイベントの配線には効かない**。反映は「保存してシーンをリロード」で行う旨を UI に明記した
- 11.（Blender にあって未実装だった trigger / delay / Clear の各条件）は
  **実装せず捨てる**方針で決着済み。Step 1 の新フォーマットから既に落としてある

**Step 4 — 作業性（完了 2026-08-04）**

| やったこと | 場所 |
|---|---|
| 編集モードと再生モードの分離、再生中の保存を禁止 | `App/Editor/AppEditor.cpp` |
| Undo / Redo（トランスフォームのみ）＋ Edit メニュー・Ctrl+Z/Y | `Engine/Editor/Core/EditorUndo.{h,cpp}` |
| ギズモと Inspector から履歴に積む | `EditorGizmo.cpp` / `InspectorWindow.cpp` |
| 並べて複製（間隔・個数を指定してまとめて配置） | `Engine/Editor/Windows/HierarchyWindow.cpp` |
| 複製がクラス固有の設定も引き継ぐ（`Editor::AddDuplicateHandler`） | `EditorHost.{h,cpp}` / `AppEditor.cpp` |

**編集モードは独立した状態を持たせず、「一時停止しているか」で判定する。**
動いている間はプレイヤーも敵も位置が変わるので、その状態の保存を許さないのが本質だから。

- 「編集モードに入る」= ステージを読み直して一時停止（遊んだ結果は捨てる）
- 「保存して再生」= 保存してから読み直して再生（編集内容は必ずファイルに残る）
- どちらもファイル経由なので、**編集内容が再生で消えることも、遊んだ結果がステージに焼き付くこともない**
- 再生中は「ステージを保存」を無効化し、理由をメニューに書いてある

Undo は**トランスフォームだけ**が対象。生成・削除まで戻すにはオブジェクトを丸ごと復元する
仕組み（ステージデータへの書き出しと読み戻し）が要り、そこまでは持たせていない。
履歴は**オブジェクトを名前で覚える**（`EditorSelection` と同じ理由）。シーンを読み直したら捨てる。

**Step 5 — 保存先の切り替え（2026-08-04）**

| やったこと | 場所 |
|---|---|
| 「いまどのステージを編集しているか」を持つ | `App/Stage/StageDocument.{h,cpp}` |
| Stage メニューに 上書き保存 / 名前を付けて保存 / 開く | `App/Editor/AppEditor.cpp` |
| `GameScene` が読むのも現在のステージ | `App/Scene/GameScene/GameScene.cpp` |

- **上書き保存** = 現在のステージへ / **名前を付けて保存** = `Resource/Stage/<名前>.json` を作って以後そちらを編集
  （同名があるときはボタンが「上書きして保存」に変わり、警告を出す）
- **開く** = `Resource/Stage` の .json を一覧から選び、切り替えて読み直す（一時停止して編集モードに入る）
- 切り替えた先は `Resource/GlobalVariables/Editor/EditorStage.json` に残るので、次の起動でも続きから編集できる。
  **記録するのは Debug ビルドだけ**（`AppEditor` が Debug 限定）。Release は常に `SceneLoader::kDefaultStagePath` を読むので、
  テスト用のステージが製品に混ざることはない
- ファイル名は `/ \ : * ? " < > |` を弾いて `Resource/Stage` の直下だけを扱う

**Player のいないステージの扱い**: `GameScene::Initialize` は `FindObject("Player")` の結果を
そのまま使っていたので、Player を置き忘れたステージでアクセス違反になっていた。
`ASSERT_MSG` で「ステージに Player がありません」と出して止めるようにした。

**残っているもの**
- **複数選択**。`EditorSelection` は単一選択のままで、ギズモも1オブジェクト前提。
  整列・ミラーも複数選択が前提になるので、まとめて後回しにした。
  「並べて複製」は単一選択で完結するので先に入れてある
- **やらないと決めたもの**: 親子構造（H。現行 Loader が既に無視している）、
  `disabled`（I。エディタ上で削除すれば済む）

---

## 参考: 触ることになるファイル

| 目的 | ファイル |
|---|---|
| 保存 | `App/Stage/SceneLoader.{h,cpp}` の隣に `SceneSaver`、`Engine/Editor/Core/EditorMenuBar.cpp` |
| クラス生成 | `App/Scene/Object3dFactory.{h,cpp}`、`Engine/Editor/Windows/HierarchyWindow.cpp` |
| クラス名保持 | `Engine/World3D/Object/Object3d.h`、`App/Stage/SceneBuilder.cpp` |
| コライダー編集 | `Engine/Editor/Windows/InspectorWindow.cpp`、`Engine/World3D/Collider/CollisionManager.h` |
| モデル差し替え | `Engine/World3D/Object/Renderer/ModelRenderer.h`、`App/GameObject/Ground/Ground.h`、`App/GameObject/Prop/Prop.h` |
| ライト | `App/GameObject/Light/StagePointLight.h`、`App/GameObject/Prop/Prop.h` |
| イベント | `App/GameObject/Event/EventFactory.{h,cpp}`、`App/GameObject/Event/*Event.h` |
| アセット一覧 | `Engine/Editor/Core/EditorAssetUtil.cpp` |
