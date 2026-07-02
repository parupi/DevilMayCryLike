# プレイヤー周り コードレビュー

調査日: 2026-07-02  
対象ブランチ: fix/playerAttackEditor

---

## 良い点

### 1. ステートパターンが適切に設計されている

`PlayerStateBase` を抽象基底にして `Enter / Update / Exit / ExecuteCommand` を統一インターフェースにし、Idle・Move・Jump・Air・Death・Clear・Knockback を完全に分離できている。新しいステートを追加するときに既存コードに触れなくてよい。

### 2. 入力が PlayerCommand に抽象化されている

`PlayerInput` がデバイス差異（ゲームパッド/キーボード）を吸収して `std::vector<PlayerCommand>` で提供しており、各ステートは「何のボタンか」を知らなくて済む設計になっている。

### 3. 通常ステートマシンと攻撃システムが分離されている

`PlayerStateMachine` と `PlayerCombat` を独立したクラスにしており、攻撃中でも Knockback のような割り込み処理が通常ステートマシン側で動く構造が成立している。

### 4. 攻撃データが完全にデータドリブン

JSON + GlobalVariables でパラメータをランタイムに変更でき、`AttackPlayer` のタイムラインビューアと組み合わせてモーション調整が実行中にできる。デバッグツールの作り込みが良い。

### 5. KnockBack ステートが独立して完結している

`pendingDamageInfo_` で情報を引き渡し、着地判定とスタン時間の両方で復帰できる。`PlayerStateKnockBack` が自己完結しており、外部から制御しなくてよい。

---

## 改善点

### 優先度: 高

---

#### 1. `OnCollisionEnter` と `OnCollisionStay` のコードが完全に重複している

**場所:** `Player.cpp:311-348` / `Player.cpp:351-388`

Ground / Enemy の衝突押し出し処理が 2 つのメソッドに丸ごとコピーされている。片方だけ直してもう片方を直し忘れるリスクがある。

**修正方針:** 共通処理を private メソッドに抽出する。

```cpp
// 追加するメソッド
void Player::ResolveAABBCollision(BaseCollider* other) { ... }

void Player::OnCollisionEnter(BaseCollider* other) {
    if (other->category_ == CollisionCategory::EnemyWeapon) { TakeDamage(...); return; }
    ResolveAABBCollision(other);
}
void Player::OnCollisionStay(BaseCollider* other) {
    ResolveAABBCollision(other);
}
```

---

#### 2. ステート識別に文字列比較を使っている

**場所:** `Player.cpp:118` / `Player.cpp:185` / `Player.cpp:264`

```cpp
// 現状 — Update() 内で毎フレーム実行されている
std::string(stateMachine_->GetCurrentState()->GetDebugName()) == "Knockback"
std::string(cur->GetDebugName()) == "Death"
```

タイポで無音バグになるうえ、毎フレームの `std::string` 構築コストも無駄。

**修正方針:** `PlayerStateBase` に型判別用の enum を持たせる。

```cpp
enum class PlayerStateID { Idle, Move, Jump, Air, Death, Clear, Knockback };

class PlayerStateBase {
public:
    virtual PlayerStateID GetID() const = 0;
    // ...
};

// 使用側
stateMachine_->GetCurrentStateID() == PlayerStateID::Knockback
```

---

### 優先度: 中

---

#### 3. `AttackStructs.h` に未使用の型が残っている

**場所:** `App/GameObject/Character/Player/State/Attack/AttackStructs.h`

以下の型が定義されているが現行コードで使われていない。

| 型名 | 競合している型 |
|---|---|
| `InputType` (Y/A/B) | `InputButton` (PlayerInput.h) |
| `StickDir` (Neutral/Up/Down/Left/Right) | `StickDirection` (PlayerCombat.h) |
| `AttackBranch` | 未使用 |
| `BranchUIElement` | 未使用 |
| `AttackInputState` | 未使用 |

現在使われているのは `AttackRequest` と `AttackRequestData` のみ。

**修正方針:** 未使用の型を削除し、`AttackRequest` / `AttackRequestData` だけを残す。

---

#### 4. `playerOffset` の計算が全方向に X 幅を流用している

**場所:** `Player.cpp:322` / `Player.cpp:361`

```cpp
// 現状 — X 幅の半分をすべての軸に使っている
float playerOffset = (playerMax.x - playerMin.x) * 0.5f;
```

AABB が正立方体でない場合、Y・Z 方向の押し出し量がずれる。

**修正方針:** 衝突法線の方向に応じた軸の半幅を使う。

```cpp
float playerOffset = 0.0f;
if (outNormal.x != 0.0f)      playerOffset = (playerMax.x - playerMin.x) * 0.5f;
else if (outNormal.y != 0.0f) playerOffset = (playerMax.y - playerMin.y) * 0.5f;
else                           playerOffset = (playerMax.z - playerMin.z) * 0.5f;
```

---

#### 5. 重力値が複数箇所にハードコードされている

**場所:** `PlayerStateAir.cpp:8` / `PlayerStateKnockBack.cpp:24`

```cpp
// Air
player.GetAcceleration().y = -12.0f;
// KnockBack
player.GetAcceleration().y = -12.0f;
```

重力を変えると両方直す必要がある。

**修正方針:** 定数として一元管理する。

```cpp
// 例: Player.h または共通ヘッダ
namespace PhysicsConst {
    inline constexpr float Gravity = -12.0f;
}
```

---

#### 6. `Player` が `attackData_` を二重に保持している

**場所:** `Player.h:124-125` / `PlayerStateAttack.cpp` 内の `attackData_`

`PlayerStateAttack` が `attackData_` を持ち、さらに `Player` も `SetAttackData` でコピーして保持している。2 つのコピーが存在するため、どちらが正として扱えば良いか曖昧になる。

**修正方針:** `Player` 側の `attackData_` を廃止し、現在の攻撃データが必要な場合は `combat_->GetCurrentAttackData()` のようなアクセサ経由にする。

---

### 優先度: 低

---

#### 7. `PlayerStateJump` が 1 フレームで通過する

**場所:** `PlayerStateJump.cpp:11`

```cpp
void PlayerStateJump::Update(Player& player, float) {
    player.ChangeState("Air"); // Enter の次のフレームで即遷移
}
```

ステートとして実質機能していない。

**修正方針:** ジャンプ踏み切りの演出（入力ホールドで高さ変化など）を実装するか、不要であれば Jump / Air を統合して `PlayerStateAir::Enter` でジャンプ速度を設定する。

---

#### 8. `AttackPlayer::Seek` の実装が不正確

**場所:** `AttackPlayer.cpp:139`

```cpp
// debugTime_ には time 全体が入っている（差分ではない）
currentAttack_->Update(*player_, debugTime_);
```

`Update` の第 2 引数は「1 フレームの差分時間」を期待しているが、`debugTime_`（= シーク先の絶対時間）を渡しているため、ステートタイマーが一気に進んでしまい正確な位置に止まらない。

**修正方針:** シーク時は `stateTime_` を直接書き換えるか、細かい dt で積算して目的時刻まで進める。

---

#### 9. 移動速度・回転速度が変更不能な定数になっている

**場所:** `Player.h:176-178`

```cpp
const float moveSpeed_   = 10.0f;
const float rotateSpeed_ = 5.0f;
```

GlobalVariables 経由で実行中に調整できない。攻撃パラメータは JSON で全部調整できるのに、基本移動パラメータだけ再コンパイルが必要になっている。

**修正方針:** `Initialize` で GlobalVariables に登録し、`Update` で毎フレーム読む（他の攻撃パラメータと同じパターン）。

---

## 影響範囲サマリー

| 優先度 | 項目 | 対応コスト |
|---|---|---|
| 高 | コード重複（OnCollision） | 関数抽出のみ、低リスク |
| 高 | 文字列によるステート判定 | PlayerStateBase 変更が必要 |
| 中 | AttackStructs.h の未使用型削除 | 削除のみ、コンパイルエラーで確認可能 |
| 中 | playerOffset の軸ずれ | 条件分岐を追加するだけ |
| 中 | 重力値の一元管理 | 定数定義のみ |
| 中 | attackData_ の二重保持 | Player 側のフィールド削除と参照先変更 |
| 低 | PlayerStateJump の整理 | 設計判断が必要 |
| 低 | AttackPlayer::Seek の修正 | デバッグツールのみ影響 |
| 低 | 移動速度の GlobalVariables 化 | 他パラメータと同じパターンで対応可能 |
