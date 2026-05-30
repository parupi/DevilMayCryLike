# Enemy システム リファクタリング変更記録

## 概要

複数種類の敵を作りやすくすることを目的に、StatePattern と Component 指向を組み合わせた設計に寄せた。
2段階で実施した。

---

## フェーズ 1：基底クラスと派生クラスの責務分離

### 背景と問題

`Enemy`（基底）と `Hellkaina`（派生）の両方が PlayerWeapon との衝突処理を持っており、
`HitDamage()` と `Hellkaina::OnCollisionEnter()` の二重実行によりダメージが2回適用されていた。

### 変更内容

#### `Enemy.h`

- `ParticleEmitter.h` のインクルードを削除
- `slashEmitter_` / `smokeEmitter_` メンバを削除（未使用、Hellkaina が独自管理するため）
- `EmitSmoke()` を削除
- `HitDamage()` を削除（Hellkaina 固有ロジックを基底クラスに持つべきでない）
- `damageInfo_` メンバを削除（`HitDamage()` と一体のため不要）
- クラスコメントを「基底クラスの責務範囲」に書き直し

#### `Enemy.cpp`

- **バグ修正**：`Update()` 内の `isAlive = false;`（存在しない変数への代入）を削除
- `OnCollisionEnter()` から PlayerWeapon 処理（`HitDamage()` 呼び出し）を削除
  - 基底クラスは地形（Ground）との AABB 衝突解決のみ担う
- `OnCollisionStay()` も同様にリファクタリング（重複コードを整理）
- `HitDamage()` メソッドを削除

#### `Hellkaina.cpp`

- `OnCollisionEnter()` を整理
  - `player_->IsAttack()` チェックを先頭に移動
  - KnockBack 中の再被弾：ヒットストップ・エフェクトのみ発火し、HP と速度は変更しない（重複被弾バグの修正）
  - HP 減算 → OnDeath 判定 → 速度計算 → KnockBack 遷移 の順序を明確化
- `OnCollisionStay()` のデッドコメントを削除

#### 責務の整理結果

| 責務 | Enemy（基底） | Hellkaina（派生） |
|---|---|---|
| ステートマシン管理 | ✅ | ステート登録のみ |
| 物理演算・接地判定 | ✅ | — |
| 地形衝突解決（AABB） | ✅ | 委譲 |
| **プレイヤー武器との衝突** | ❌ 削除 | ✅ 完全に担当 |
| HP・死亡管理 | ✅ | HP 初期値のみ設定 |
| 武器・エフェクト | — | ✅ |

---

## フェーズ 2：拡張性向上（State Pattern + Component 指向）

### 洗い出した問題点

| 問題 | 詳細 |
|---|---|
| `EnemyStateIdle` の `static` 変数 | 複数インスタンスでクールタイムが共有される（バグ） |
| `ChangeState()` に `DamageInfo*` + `dynamic_cast` | KnockBack だけ特別扱い。`Enter()` が実際には呼ばれていなかった（バグ） |
| `AttackA/B` のコード重複 + `static_cast<Hellkaina&>` | 攻撃ステートを別の敵に転用できない |
| ステート名が文字列リテラル散在 | タイプミスがランタイムエラーになる |
| `hitStop_` が派生クラス任せ | 新しい敵を作るとクラッシュのリスク |
| `DeltaTime::GetDeltaTime()` を直接使用 | ヒットストップが SideMove・Attack に効かない |
| AI パラメータのハードコード | Idle ステートを他の敵に転用できない |
| `Object3dFactory` が if-else チェーン | 敵を追加するたびにファクトリを編集する必要がある |

---

### 新規ファイル

#### `App/GameObject/Character/Enemy/EnemyStateNames.h`（新規作成）

ステート名の文字列をコンパイル時定数として管理。タイプミスをランタイムエラーから
コンパイルエラーに変換する。

```cpp
namespace EnemyStateName {
    constexpr const char* Air       = "Air";
    constexpr const char* Idle      = "Idle";
    constexpr const char* Move      = "Move";
    constexpr const char* KnockBack = "KnockBack";
}

namespace HellkainaStateName {
    constexpr const char* SideMove = "SideMove";
    constexpr const char* AttackA  = "AttackA";
    constexpr const char* AttackB  = "AttackB";
}
```

---

### 変更ファイル詳細

#### `Enemy.h / Enemy.cpp`

**`ChangeState()` のシグネチャ統一**

`DamageInfo*` パラメータと `dynamic_cast<EnemyStateKnockBack*>` を削除。
全ステートが同じ `Enter(Enemy&)` シグネチャで呼ばれるようになった。

```cpp
// 変更前
void ChangeState(const std::string& stateName, const DamageInfo* info = nullptr);
// 内部で dynamic_cast<EnemyStateKnockBack*> して分岐

// 変更後
void ChangeState(const std::string& stateName);
// 全ステートを Enter(enemy) で統一呼び出し
```

**`pendingDamageInfo_` の追加**

KnockBack ステートが必要とするダメージ情報を `Enemy` に保持。
ステートは `Enter()` 内で `enemy.GetPendingDamageInfo()` から取得する。

```cpp
void SetPendingDamageInfo(const DamageInfo& info) { pendingDamageInfo_ = info; }
const DamageInfo& GetPendingDamageInfo() const { return pendingDamageInfo_; }
```

**`hitStop_` の安全化**

`Enemy::Initialize()` でデフォルト初期化することで、派生クラスが生成を忘れてもクラッシュしない。

```cpp
void Enemy::Initialize() {
    if (!hitStop_) {
        hitStop_ = std::make_unique<HitStop>();
    }
    // ...
}
```

---

#### `BaseState/EnemyStateKnockBack.h`

旧来の別シグネチャ `Enter(const DamageInfo& info, Enemy& enemy)` を廃止。
`EnemyStateBase::Enter(Enemy&)` と統一することで `dynamic_cast` が不要になった。

```cpp
// 変更前
class EnemyStateKnockBack : public EnemyStateBase {
public:
    virtual void Enter(const DamageInfo& info, Enemy& enemy) = 0;
};

// 変更後
class EnemyStateKnockBack : public EnemyStateBase {
    // 追加メンバなし。Enter() は EnemyStateBase の仮想関数をそのまま使う
};
```

---

#### `HellkainaStateKnockBack.h / .cpp`

`Enter()` シグネチャを `EnemyStateBase` に合わせ、内部で `GetPendingDamageInfo()` を使うように変更。

```cpp
// 変更前
void Enter(const DamageInfo& info, Enemy& enemy) override;

// 変更後
void Enter(Enemy& enemy) override;
// 実装内で const DamageInfo& info = enemy.GetPendingDamageInfo(); を呼ぶ
```

また `ChangeState("Idle")` を `ChangeState(EnemyStateName::Idle)` に定数化。

---

#### `EnemyStateIdle.h / .cpp`

**`static` 変数バグの修正**

複数の Hellkaina インスタンスでクールタイムが共有されていた問題を修正。

```cpp
// 変更前（バグ）
static bool isAttackCooldown = false;  // 全インスタンス共有
static float cooldownTimer = 0.0f;     // 全インスタンス共有

// 変更後（.h に追加）
bool isAttackCooldown_ = false;  // インスタンスごとに独立
float cooldownTimer_ = 0.0f;
```

**AI パラメータをファイルスコープ定数に整理**

行動確率・距離しきい値を anonymous namespace 内の `constexpr` 定数として明示。
値の意図と関係性が読みやすくなった。

```cpp
namespace {
    constexpr float kNearDistance   = 3.0f;
    constexpr float kMidDistance    = 16.0f;
    constexpr float kAttackCooldown = 1.0f;

    constexpr int kNear_AttackA  = 10; //  0〜 9: AttackA
    constexpr int kNear_SideMove = 20; // 10〜19: SideMove / 残り: Move

    constexpr int kMid_AttackB   =  5; //  0〜 4: AttackB
    constexpr int kMid_SideMove  = 30; //  5〜29: SideMove / 残り: Move

    constexpr int kFar_SideMove  = 20; //  0〜19: SideMove / 残り: Move
}
```

ステート名文字列を `EnemyStateName::` / `HellkainaStateName::` 定数に置き換え。

---

#### `HellkainaStateAttackA.h / .cpp` → `HellkainaWeaponAttackState` に統合

AttackA と AttackB はモーションデータと移動速度以外が完全に同一だったため、
`HellkainaWeaponAttackState` として統合。ファイル名はそのまま維持（vcxproj 変更なし）。

**`static_cast<Hellkaina&>` の除去**

武器ポインタをコンストラクタで受け取ることで、型を強制変換せずに武器を操作できる。

```cpp
// 変更前（型安全でない）
Hellkaina& hellkaina = static_cast<Hellkaina&>(enemy);
hellkaina.GetWeapon()->GetWorldTransform()->...

// 変更後（weapon をコンストラクタで受け取る）
class HellkainaWeaponAttackState : public EnemyStateBase {
    HellkainaWeaponAttackState(HellkainaWeapon* weapon, WeaponMotionParams params);
    // Update() 内で weapon_-> を直接使う
};
```

**`WeaponMotionParams` 構造体の導入**

モーションデータをデータとして外部から注入できる形に。

```cpp
struct WeaponMotionParams {
    float duration;
    float moveSpeed;
    std::vector<Vector3> translate;  // Catmull-Rom 制御点
    std::vector<Vector3> rotate;
};
```

**`HellkainaStateAttackB.h/.cpp`** はヘッダのインクルードのみに縮小（AttackA ヘッダを参照）。

---

#### `HellkainaStateSideMove.cpp` / `EnemyStateMove.cpp` / `EnemyStateAir.cpp`

`DeltaTime::GetDeltaTime()` を直接呼んでいた箇所を、引数の `deltaTime` に統一。
これにより、`hitStop_` による時間スケールがこれらのステートにも正しく適用されるようになった。

```cpp
// 変更前（ヒットストップが効かない）
timer_ += DeltaTime::GetDeltaTime();

// 変更後（Enemy::Update() からヒットストップ適用済みの dt が渡される）
timer_ += deltaTime;
```

ステート名文字列を `EnemyStateName::` 定数に置き換え。

---

#### `Hellkaina.h / .cpp`

**ステート生成を `Initialize()` に移動**

コンストラクタで weapon より先にステートを生成していたため、AttackA/B に weapon ポインタを渡せなかった。
`Initialize()` に移動することで weapon 確定後にステートを生成できる。

```cpp
// コンストラクタ（変更後）：レンダラー設定と HP のみ
Hellkaina::Hellkaina(std::string objectName) : Enemy(objectName)
{
    hp_ = 10.0f;
}

// Initialize()（変更後）：weapon 確定 → ステート生成の順序が保証される
void Hellkaina::Initialize()
{
    // ... weapon を生成 ...
    weapon_ = weapon.get();  // ← ここで確定

    // weapon が確定したのでステートに渡せる
    states_[HellkainaStateName::AttackA] =
        std::make_unique<HellkainaWeaponAttackState>(weapon_, MakeAttackAParams());
    states_[HellkainaStateName::AttackB] =
        std::make_unique<HellkainaWeaponAttackState>(weapon_, MakeAttackBParams());
}
```

**`OnCollisionEnter()` で `DamageInfo` を構築して渡すように変更**

```cpp
DamageInfo info;
info.direction    = Normalize(hitPos - attackerPos);
info.type         = player_->GetAttackData().type;
// ... 各フィールドを設定 ...

SetPendingDamageInfo(info);
ChangeState(EnemyStateName::KnockBack);  // Enter() 内で GetPendingDamageInfo() を参照
```

**モーションパラメータを名前付き関数として切り出し**

```cpp
namespace {
    WeaponMotionParams MakeAttackAParams() { /* 近距離薙ぎのデータ */ }
    WeaponMotionParams MakeAttackBParams() { /* 高速突進のデータ */ }
}
```

---

#### `Object3dFactory.h / .cpp`

if-else チェーンを登録ベースのファクトリに変更。

```cpp
// 変更前：敵を追加するたびにファクトリ本体を編集する必要があった
std::unique_ptr<Object3d> Object3dFactory::Create(...) {
    if (className == "Player") { ... }
    else if (className == "HellKaina") { ... }
    else if (className == "NewEnemy") { ... }  // ← ここを毎回追加
}

// 変更後：登録と生成を分離
using Creator = std::function<std::unique_ptr<Object3d>(const std::string&)>;
static void Register(const std::string& className, Creator creator);
static std::unique_ptr<Object3d> Create(const std::string& className, const std::string& objectName);
// Create() は Registry を引くだけ。ファクトリ本体への変更は不要
```

#### `GameScene.cpp`

`Initialize()` の冒頭でクラス登録を行うようにした。

```cpp
Object3dFactory::Register("Player",    [](const std::string& n){ return std::make_unique<Player>(n); });
Object3dFactory::Register("HellKaina", [](const std::string& n){ return std::make_unique<Hellkaina>(n); });
Object3dFactory::Register("Ground",    [](const std::string& n){ return std::make_unique<Ground>(n); });
```

---

## 次の敵を追加するときの手順

### 1. クラスを作成

```cpp
// NewEnemy.h
class NewEnemy : public Enemy
{
public:
    NewEnemy(std::string objectName);
    void Initialize() override;
    void OnCollisionEnter(BaseCollider* other) override;
    // ...
};
```

### 2. `Initialize()` でステートを登録

```cpp
void NewEnemy::Initialize()
{
    // 共通ステートはそのまま使える
    states_[EnemyStateName::Idle]      = std::make_unique<EnemyStateIdle>();
    states_[EnemyStateName::Move]      = std::make_unique<EnemyStateMove>();
    states_[EnemyStateName::Air]       = std::make_unique<EnemyStateAir>();
    states_[EnemyStateName::KnockBack] = std::make_unique<NewEnemyStateKnockBack>();

    // 固有ステートは独自 namespace で定義
    states_[NewEnemyStateName::Charge] = std::make_unique<NewEnemyStateCharge>();

    Enemy::Initialize();
}
```

### 3. `OnCollisionEnter()` で被弾処理を実装

```cpp
void NewEnemy::OnCollisionEnter(BaseCollider* other)
{
    Enemy::OnCollisionEnter(other);  // 地形衝突は委譲

    if (other->category_ != CollisionCategory::PlayerWeapon) return;
    if (!player_ || !player_->IsAttack()) return;

    hp_ -= player_->GetAttackData().damage;
    if (hp_ <= 0.0f) OnDeath();

    DamageInfo info;
    // ... info を構築 ...
    SetPendingDamageInfo(info);
    ChangeState(EnemyStateName::KnockBack);
}
```

### 4. ファクトリに1行登録するだけ

```cpp
// GameScene::Initialize() に追加するだけでいい
Object3dFactory::Register("NewEnemy",
    [](const std::string& n){ return std::make_unique<NewEnemy>(n); });
```
