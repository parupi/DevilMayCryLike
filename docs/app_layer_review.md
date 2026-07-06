# App層 アーキテクチャ評価

調査日: 2026-07-06
対象ブランチ: feature/tutorial

---

## 緊急: App/Scene と App/scene の重複ディレクトリ問題

`GameScene.cpp` などが大文字パス (`App/Scene/...`) と小文字パス (`App/scene/...`) の両方に git 管理されており、内容も一致する完全な重複ファイルになっている。

- 現在の作業ツリーで**両方が別々に編集されており、既に内容が乖離し始めている**（`App/scene/GameScene/State/GameSceneStatePlay.cpp` は編集されたが、大文字側の対応ファイルは編集されていない）。
- `GuchisEngine.vcxproj` は大文字パス (`App\Scene\...`) のみを参照しているため、小文字側は現状ビルド対象から外れている。
- しかし `premake5.lua` は `App/**.cpp` をグロブしているため、**premake を再実行するとシンボル重複でリンクエラーになる地雷**になっている。
- Windows のファイルシステムが大文字小文字を区別しないために起きた事故と推測される。

**優先度: 最高。** どちらかのディレクトリを削除し、パスを統一することを推奨する。

---

## モジュール別 良い点・課題点（粒度が大きい順）

### 1. GameObject/Character/Player（約2,666行・28ファイル）

**良い点**
- `Player`本体（195行）は `PlayerCombat` / `PlayerStateMachine` / `PlayerWeapon` / `HitStop` へきちんと委譲しており、State数8種（Idle/Move/Jump/Air/KnockBack/Death/Clear/Attack）も整理されている。

**課題点**
- `Player.h` が `LockOnSystem` / `TutorialService` / `GlobalVariables` シングルトンに直接依存し、UI用の `std::vector<Sprite*> hearts_` まで抱えており、入力/UI/チュートリアル/スコアの関心が1クラスに集中している。
- `PlayerCombat.h` が `<imgui.h>` を直接includeし、デバッグ用エディタメソッド（`DrawAttackDataEditorUI` 等）とゲームプレイ用メソッドが同じ公開インターフェースに混在している。
- `PlayerStateAttack.cpp`（327行）は `GlobalVariables::GetInstance().GetValueRef<...>()` を約20回呼び出して攻撃データを都度取得しており、構造体化されていない。
- コメントアウトされた死コードが散見される（`Player.h:134, 176`）。

### 2. GameObject/Character/Enemy（約2,789行）

**良い点**
- State（意思決定）とComponent（Sensor/Movement/MeleeAttack、実行部）が明確に分離されており、GruntMeleeとBossKnightで実際にコンポーネントを共有できている（コピペではない）。

**課題点**
- Hellkainaだけ専用Stateフォルダを持たず、共通の `Enemy/State/` に個別State（`HellkainaStateAttackA/B`）を直置きしており構成が不統一。
- `HellkainaStateAttackB` は3〜4行の空スタブで、未実装のまま放置されている可能性がある。
- 同じ「確率で行動選択」の実装が、Gruntでは名前付き定数（`kNear_AttackNormal=40`）、Bossでは生の数値リテラル（`dist < 4.0f`, `roll < 60`）と、兄弟クラス間で流儀が揃っていない。

### 3. Scene（GameScene他）

**良い点**
- `GameScene/State/`（Start/Menu/Play/Clear）はシーンフェーズの小さなStateパターンとして妥当に機能している。

**課題点**
- 前述の重複ディレクトリ問題。
- `GameScene.cpp`（204行）の `Initialize()` が全リソースロードの寄せ集めになっており（`TextureManager::GetInstance()` 19回、`ParticleManager` 10回、`ModelManager` 9回呼び出し）、ゴッドオブジェクト気味。

### 4. Tutorial

**良い点**
- `TutorialService`（薄いインターフェース）→ `TutorialSystem`（進行管理）→ `Tutorial`（個別状態）→ `TutorialDecoration`（演出）と、責務ごとにきれいに層になっている。

**課題点**
- `TutorialSystem.cpp:18-22` で5種類のチュートリアル設定が全て同じ画像 `"PlayerWalk"` を指定しており、未実装のプレースホルダーが残っている。
- `LockOnSystem::Update()` が `player_->GetTutorialService()->StepTutorial(...)` を直接呼んでおり、ロックオン機能にチュートリアル進行ロジックが漏れ出している。

### 5. UI / Event / LockOn / Camera / Effect / Ground

**良い点**
- `EventFactory::Create` は文字列switchのみの14行で責務が明確。
- `GameCamera` もFree/LockOnモードの委譲構造が単純明快。

**課題点**
- `EventManager::AddEvent` が生ポインタに対して `std::move` を呼ぶ意味のないコードになっており、`FindEvent` は `events_[eventName]`（`operator[]`）を使っているため**検索するだけでnullエントリをmapに挿入してしまう実バグ**がある（`EventManager.cpp:17-34`）。
- `LockOnSystem::CalculateScore` が画面解像度 `1280, 720` をハードコードしており、コメントアウトされたデバッグ描画も残存している。
- `GameCamera` のチューニング値（`yaw_=3.14f`, `distance_=18.0f` 等）がメンバ初期値に直書きされ、外部データ化されていない。

### 6. GameData / Input / Stage（最小粒度）

**良い点**
- `InputContext`（43行）は `PlayerInput` / `LockOnInput` / `CameraInput` を許可フラグ付きで束ねるだけの薄いクラス。
- `SceneBuilder` も全静的メソッドで単一責務。

**課題点**
- `GameData`（スコア/ランク保持）と別に `StylishScoreManager` という並行したスコア管理クラスが存在し、両者の連携が見えない＝スコア概念が2系統に分裂している。

---

## 全体傾向と優先度

個々のモジュール設計自体（特にEnemyのState+Component分離）は筋が良いが、以下の課題が繰り返し出ている。

- シングルトンマネージャー（`GlobalVariables`, 各種`Manager::GetInstance()`）への直接依存
- デバッグ用コードと本番ロジックの混在（`imgui.h`の直接include等）
- 同じ問題への解法が兄弟クラス間で不統一（定数化 vs 生の数値リテラル）

**対応優先度（推奨）**

1. `App/Scene` と `App/scene` の重複解消（ビルド破綻の地雷）
2. `EventManager::FindEvent` の実バグ修正（`operator[]` による意図しないnull挿入）
3. `GameData` と `StylishScoreManager` のスコア管理統合
