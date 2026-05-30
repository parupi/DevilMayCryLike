# GruntMelee 近接攻撃型敵 設計・実装まとめ

作成日: 2026-05-31  
ブランチ: `feature/EnemySystem`

---

## 概要

プレイヤーを発見するまで待機し、発見後は距離に応じて移動・攻撃を切り替える近接型の敵。

| 責務 | 担当 |
|------|------|
| **意思決定**（何をするか選ぶ） | State |
| **行動実行**（どう動くか） | Component |

---

## ファイル構成

```
App/GameObject/Character/Enemy/
├── EnemyStateNames.h              ← GruntMeleeStateName 名前空間を追加
│
├── Component/
│   ├── EnemySensorComponent.h/cpp     ← プレイヤー感知
│   ├── EnemyMovementComponent.h/cpp   ← 移動実行（新規）
│   └── EnemyMeleeAttackComponent.h/cpp← 武器モーション・攻撃管理（新規）
│
└── GruntMelee/
    ├── GruntMelee.h/cpp               ← 敵本体
    ├── GruntMeleeWeapon.h/cpp         ← 武器オブジェクト
    └── State/
        ├── GruntStatePatrol.h/cpp         ← 発見前待機
        ├── GruntStateCombatIdle.h/cpp     ← 行動選択（意思決定の中核）
        ├── GruntStateApproach.h/cpp       ← 接近
        ├── GruntStateSideMove.h/cpp       ← 左右移動
        ├── GruntStateRetreat.h/cpp        ← 後退
        ├── GruntStateAttackNormal.h/cpp   ← 通常攻撃
        └── GruntStateRushAttack.h/cpp     ← 突進攻撃
```

---

## Component（行動実行）

### EnemySensorComponent

プレイヤーの感知を担当する。毎フレーム `Update()` を呼ぶことで距離・方向・発見フラグを更新する。

```cpp
sensor_->Update(enemy);
sensor_->IsPlayerDetected();     // 発見済みか
sensor_->GetDistanceToPlayer();  // 距離
sensor_->GetDirectionToPlayer(); // 正規化ベクトル
sensor_->SetDetectionRange(12.0f); // 感知距離（デフォルト 12m）
```

- 一度発見すると `detected_` は `true` のまま（見失い判定なし）

---

### EnemyMovementComponent

敵の `velocity` に書き込む形で移動を実行する。

| メソッド | 動作 |
|---------|------|
| `MoveToward(enemy, speed, stopDist)` | プレイヤーへ接近。`stopDist` 以内は止まる |
| `MoveAway(enemy, speed)` | プレイヤーから後退 |
| `MoveSideways(enemy, speed, dir)` | プレイヤーを軸に横移動（`dir: +1` 右 / `-1` 左） |
| `Stop(enemy)` | XZ 速度をゼロに（Y は維持） |

---

### EnemyMeleeAttackComponent

攻撃を **予備動作（Windup）** と **攻撃（Attack）** の2フェーズで実行する。

```
BeginAttack(enemy, params) を呼ぶと開始

  ── Windup フェーズ (windupDuration 秒) ──────────────
  ・武器を t=0 のキーフレームポーズで固定（構え）
  ・敵の XZ 速度を 0 にして静止（溜め動作）

  ── Attack フェーズ (attackDuration 秒) ──────────────
  ・CatmullRomSpline で武器を高速スイング
  ・rushSpeed > 0 のとき、攻撃フェーズ中だけ突進

IsFinished() が true になったら終了
```

#### MeleeAttackParams

| フィールド | 説明 |
|-----------|------|
| `windupDuration` | 予備動作の保持時間（秒） |
| `attackDuration` | 攻撃モーションの時間（秒） |
| `rushSpeed` | 突進速度（0 なら突進なし） |
| `weaponTranslate` | 武器位置キーフレーム（4点 CatmullRom） |
| `weaponRotate` | 武器回転キーフレーム（4点 CatmullRom） |

> **キーフレーム構造**  
> `[0]` ghost → `[1]` t=0（構えポーズ） → `[2]` t=1（振り切り） → `[3]` ghost  
> Windup 中は `[1]` が武器の見た目になる。

---

## State（意思決定）

### ステート一覧

| ステート | キー | コンポーネント | 役割 |
|---------|------|---------------|------|
| `GruntStatePatrol` | `Patrol` | Sensor | 発見前待機。感知 → CombatIdle |
| `GruntStateCombatIdle` | `CombatIdle` / `Idle` / `Move` | Sensor | **行動選択**。距離・乱数で次ステートへ |
| `GruntStateApproach` | `Approach` | Movement | 接近（1.2秒） |
| `GruntStateSideMove` | `SideMove` | Movement | 左右移動（1.0秒、ランダム方向） |
| `GruntStateRetreat` | `Retreat` | Movement | 後退（0.8秒） |
| `GruntStateAttackNormal` | `AttackNormal` | MeleeAttack | 通常攻撃（溜め0.55s→振り0.22s） |
| `GruntStateRushAttack` | `RushAttack` | MeleeAttack | 突進攻撃（溜め0.65s→突進0.30s） |

> `EnemyStateName::Idle` / `EnemyStateName::Move` にも `CombatIdle` を登録している。  
> `EnemyStateKnockBack` や `EnemyStateAir` が終了後にこれらへ遷移するため。

---

### GruntStateCombatIdle — 行動確率

距離によって行動の確率分布が変わる。

#### 近距離（< 3m）

| 行動 | 確率 |
|------|------|
| 通常攻撃 | 40% |
| 左右移動 | 20% |
| 後退 | 20% |
| 接近 | 20% |

#### 中距離（3〜8m）

| 行動 | 確率 |
|------|------|
| 突進攻撃 | 30% |
| 接近 | 30% |
| 左右移動 | 20% |
| 通常攻撃 | 20% |

#### 遠距離（> 8m）

| 行動 | 確率 |
|------|------|
| 接近 | 60% |
| 左右移動 | 30% |
| 後退 | 10% |

行動選択後は 0.8 秒のクールダウン（次の選択まで待機）。

---

### ステート遷移図

```
[Patrol] ─ 感知 ──────────────────────────→ [CombatIdle]
                                                  │
                     ┌────────────────────────────┤
                     │  距離・乱数で選択           │
                     ↓                             │
          [Approach] → 1.2s → ──────────────→     │
          [SideMove] → 1.0s → ──────────────→     │
          [Retreat]  → 0.8s → ──────────────→ [CombatIdle]
          [AttackNormal] → 終了 → ──────────→     │
          [RushAttack]   → 終了 → ──────────→     │
                                                  │
          ※ 被弾 → [KnockBack] → 終了 → [CombatIdle]
          ※ 落下 → [Air]       → 着地 → [CombatIdle]
```

---

## 攻撃の予備動作（見た目）

視覚的なわかりやすさのため、2段階モーションを採用。

### 通常攻撃
```
【構え 0.55s】武器を頭上・後方に大きく引いて静止
      ↓
【振り 0.22s】素早く振り下ろす
```

### 突進攻撃
```
【溜め 0.65s】武器を横・後方に大きく引いて静止（前進停止）
      ↓
【突進 0.30s】rushSpeed=18 で高速突進しながら振り抜く
```

> 溜め中は敵が静止するため「次に攻撃が来る」ことがプレイヤーに伝わりやすい。

---

## 配置方法

Stage JSON のオブジェクト定義で `className` を指定する。

```json
{
  "className": "GruntMelee",
  "name": "GruntMelee_01",
  "position": [5.0, 0.0, 10.0]
}
```

ファイル追加後は **premake5 を再実行**して VS プロジェクトを再生成すること。

---

## 調整パラメータ

| 場所 | パラメータ | 内容 |
|------|-----------|------|
| `EnemySensorComponent.h` | `detectionRange_` | 感知距離（デフォルト 12m） |
| `GruntStateCombatIdle.cpp` | `kNearDist` / `kMidDist` | 距離しきい値 |
| `GruntStateCombatIdle.cpp` | `kCooldown` | 行動選択インターバル（秒） |
| `GruntStateCombatIdle.cpp` | `kNear_*` / `kMid_*` / `kFar_*` | 行動確率のしきい値 |
| `GruntStateApproach.cpp` | `kSpeed` / `kStopDist` | 接近速度・停止距離 |
| `GruntStateRetreat.cpp` | `kSpeed` | 後退速度 |
| `GruntStateAttackNormal.cpp` | `windupDuration` / `attackDuration` | 予備動作・攻撃時間 |
| `GruntStateRushAttack.cpp` | `windupDuration` / `rushSpeed` | 溜め時間・突進速度 |

---

## 今回修正した既存ファイル

| ファイル | 修正内容 |
|---------|---------|
| `EnemyStateNames.h` | `GruntMeleeStateName` 名前空間を追加 |
| `Hellkaina.cpp` | 存在しない `HellkainaStateKnockBack` / `HellkainaStateSideMove` のインクルードを `EnemyStateKnockBack` / `EnemyStateSideMove` に修正 |
| `GameScene.cpp` | `Object3dFactory::Register("GruntMelee", ...)` を追加 |
