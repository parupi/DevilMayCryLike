# プレイヤー: エディター設定と挙動の不一致

調査日: 2026-06-29  
対象ブランチ: fix/playerAttackEditor

---

## 問題1: エディターで設定できるが、コードで完全に無視されている項目

### 概要

`AttackNode` 構造体にフィールドが存在せず、`PlayerCombat::ExecuteCommand` のロジックもこれらの値を参照しない。エディターで何を設定しても攻撃遷移に影響しない。

`PlayerCombat.h:30-34` に `InputRequirement` 構造体（`button`, `requireLockOn`, `stick`）が定義済みだが、`AttackNode` に組み込まれておらずデータドリブンな入力判定が未実装の状態。

### 該当項目

| エディター表示 | GlobalVariables キー | 登録箇所 | 実際の挙動 |
|---|---|---|---|
| **Button** (None/X/Y) | `ButtonIndex` | `PlayerStateAttack.cpp:61` | `ExecuteCommand` は `PlayerAction::Attack` を一括チェックするだけでX/Yボタンを区別しない (`PlayerCombat.cpp:111`) |
| **RequireLockOn** | `LockOnFlag` | `PlayerStateAttack.cpp:62` | ロックオン判定はルート攻撃時のみ `player_->IsLockOn()` でハードコード (`PlayerCombat.cpp:117`)。各攻撃ノードの値は読まれない |
| **Stick Direction** | `DirIndex` | `PlayerStateAttack.cpp:65` | スティック判定は `stickDir.y <= -0.7f` / `>= 0.7f` のハードコード (`PlayerCombat.cpp:119,123`)。各攻撃の `DirIndex` は無視 |
| **IsAir** | `IsAir` | `PlayerStateAttack.cpp:64` | 地上/空中の判定は `player_->GetOnGround()` 直接参照 (`PlayerCombat.cpp:131,134`)。各攻撃の `IsAir` フラグは未使用 |
| **IsRootAttack** | `RootAttackFlag` | `PlayerStateAttack.cpp:63` | コード全体で `GetValueRef("RootAttackFlag")` による読み取りが一切ない |

### 修正方針

`AttackNode` に `InputRequirement` を持たせ、`LoadAttackNode` でこれらの値を読み込む。`ExecuteCommand` でコマンドと各ノードの入力条件を照合するデータドリブンな遷移ロジックに置き換える。

---

## 問題2: GlobalVariables に登録されているが参照されないフィールド

| キー | 登録箇所 | 問題 |
|---|---|---|
| `"Posture"` | `PlayerStateAttack.cpp:35` | `AddItem` で登録されているが、コード内で `GetValueRef("Posture")` による読み取りが一切ない。実際に使われているのは同ファイル:45 の `"AttackPosture"` のみ |

### 修正方針

`PlayerStateAttack.cpp:35` の `gv->AddItem(name_, "Posture", int32_t());` を削除する。既存のJSONファイルに `"Posture"` キーが残っている場合も合わせて削除する。

---

## 問題3: エディター表示名とキー名が食い違っている

| エディター表示ラベル | 実際に読み書きされるキー | 場所 |
|---|---|---|
| `"Posture:"` (RadioButton グループのラベル) | `"AttackPosture"` | `PlayerCombat.cpp:359` でラベルは `"Posture:"` と表示されるが、RadioButton が操作するのは `"AttackPosture"` キー |

### 修正方針

混同を防ぐためラベルを `"AttackPosture:"` に統一するか、`"Posture"` キーを削除して `"AttackPosture"` に一本化する（問題2と同時対応推奨）。

---

## 影響範囲サマリー

| 優先度 | 問題 | 対応コスト |
|---|---|---|
| 高 | 入力条件5項目がすべて無視される（問題1） | `AttackNode` 拡張 + `ExecuteCommand` 書き換えが必要 |
| 低 | `"Posture"` キーのデッドコード（問題2） | 1行削除 + JSON清掃 |
| 低 | ラベルとキー名の不一致（問題3） | 1行修正 |
