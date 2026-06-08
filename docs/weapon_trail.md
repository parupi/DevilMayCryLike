# WeaponTrail 実装記録

## 概要

武器攻撃中の軌跡エフェクトをパーティクル（点の集積）からトレイル（リボンメッシュ）に変更した。  
毎フレーム刃先と根本の2点を記録し、それらをつないだ帯状のポリゴン（TriangleStrip）をリアルタイムに生成して描画する。

---

## 変更前との比較

| 項目 | 変更前（パーティクル） | 変更後（トレイル） |
|------|----------------------|--------------------|
| 描画方式 | 無数の小さな板ポリゴンをインスタンシング描画 | 1枚のリボンメッシュを TriangleStrip で描画 |
| 見た目 | 点が散らばったエフェクト | 刃の軌跡が連続した帯として見える |
| 設定 | `WeaponTrailEmitter.json` / `WeaponTrailEffect.json` | `WeaponTrail` クラスのメンバ変数で調整 |
| ブレンド | 加算合成 | 加算合成（同じ） |

---

## 新規作成ファイル

### シェーダー

#### `Resource/shaders/Trail.VS.hlsl`

```hlsl
cbuffer TrailCB : register(b0) { float4x4 viewProj; float4 tintColor; };
```

- 頂点入力: `POSITION(float3)` + `TEXCOORD(float2)` + `COLOR(float4)`
- ワールド座標の頂点を ViewProjection 行列で変換する
- 頂点カラーと `tintColor` を乗算してピクセルシェーダーへ渡す

#### `Resource/shaders/Trail.PS.hlsl`

- `t0` テクスチャをサンプリングして頂点カラーと乗算
- α値が `0.01` 以下なら `discard`

---

### エンジン層

#### `Engine/3d/Trail/WeaponTrail.h` / `WeaponTrail.cpp`

トレイルの本体クラス。`PlayerWeapon` が所有する。

**主なメンバ**

| メンバ | 型 | 説明 |
|--------|----|----|
| `points_` | `std::deque<TrailPoint>` | サンプル点の履歴（最大 32 点） |
| `lifetime_` | `float` | 各点の残存時間（デフォルト 0.25 秒） |
| `tintColor_` | `Vector4` | トレイル全体の色合い（デフォルト 青白） |
| `vbHandle_` | `BufferHandle` | Upload ヒープの動的頂点バッファ |
| `cbHandle_` | `BufferHandle` | 定数バッファ（viewProj + tintColor） |
| `trailTexture_` | `ComPtr<ID3D12Resource>` | グラデーションテクスチャ（実行時生成） |

**主なメソッド**

| メソッド | 呼び出し元 | 説明 |
|---------|-----------|------|
| `Initialize()` | `PlayerWeapon::Initialize()` | VB・CB・テクスチャを生成 |
| `AddPoint(tip, hilt)` | `PlayerWeapon::Update()` | 攻撃中に毎フレーム刃先と根本のワールド座標を追加 |
| `Update(dt)` | `PlayerWeapon::Update()` | `age` を進めて寿命切れの点を削除 |
| `Clear()` | 任意 | 軌跡を即座に消す |
| `Draw()` | `PlayerWeapon::DrawEffect()` | リボンメッシュを構築して描画 |

**リボンメッシュの構築（`BuildMesh`）**

```
points_[0](古) ─── points_[1] ─── ... ─── points_[N-1](新)

各 points_[i] から 2 頂点を生成:
  tip 頂点  : position=tip,  uv=(u, 0.0), color=(1,1,1, u)
  hilt 頂点 : position=hilt, uv=(u, 1.0), color=(1,1,1, u)

  u = i / (N-1)  ← 0=古い端(透明), 1=新しい端(不透明)

頂点順: tip[0], hilt[0], tip[1], hilt[1], ..., tip[N-1], hilt[N-1]
D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP で描画 → N-1 個のクワッドが連結
```

**グラデーションテクスチャ（64×8 RGBA8, 実行時生成）**

```
alpha(u, v) = u * sqrt(1 - (2v-1)^2)
  U 方向: 0(古い端=透明) → 1(新しい端=不透明)
  V 方向: 端(0/1)=やや透明, 中央(0.5)=不透明 → ソフトエッジ効果
RGB = (255, 255, 255)  ← tintColor で色合いを制御
```

`UploadTextureData()` が内部で `COPY_DEST → GENERIC_READ` へのバリアを発行するため、
呼び出し側での手動バリアは不要。

---

### PSO

#### `Engine/Graphics/Rendering/PSO/PSOManager.h` / `PSOManager.cpp`

Trail 専用の RootSignature と PSO を追加。

**RootSignature（`CreateTrailSignature`）**

| スロット | 種別 | レジスタ | 内容 |
|---------|------|---------|------|
| `[0]` | CBV | b0 (ALL) | viewProj (Matrix4x4) + tintColor (Vector4) |
| `[1]` | DescriptorTable | t0 (PS) | テクスチャ SRV |
| Static Sampler | — | s0 (PS) | Linear, Clamp |

**PSO 設定（`CreateTrailPSO`）**

| 設定 | 値 |
|------|----|
| VS / PS | `Trail.VS.hlsl` / `Trail.PS.hlsl` |
| InputLayout | POSITION(R32G32B32) + TEXCOORD(R32G32) + COLOR(R32G32B32A32) |
| BlendMode | 加算合成（SrcAlpha × Src + 1 × Dest） |
| DepthWrite | なし（`DEPTH_WRITE_MASK_ZERO`） |
| DepthTest | あり（`LESS_EQUAL`） |
| CullMode | なし（`CULL_MODE_NONE`） |
| Topology | `TRIANGLE`（実際のトポロジは描画時に `TRIANGLESTRIP` を指定） |
| RTV Format | `R16G16B16A16_FLOAT` |

アクセッサ: `GetTrailPSO()` / `GetTrailSignature()` を追加。

---

### RendererManager

#### `Engine/3d/Object/Renderer/RendererManager.h`

```cpp
PSOManager* GetPsoManager() { return psoManager_; }
```

`WeaponTrail::Draw()` が PSO にアクセスするために追加。

---

## 変更ファイル（アプリ層）

### `App/GameObject/Character/Player/PlayerWeapon.h`

- `ParticleEmitter* emitter_` を削除
- `std::unique_ptr<WeaponTrail> trail_` を追加
- 刃先・根本のローカルオフセットを追加:
  ```cpp
  Vector3 tipOffset_  = { 0.0f,  0.5f, 0.0f };
  Vector3 hiltOffset_ = { 0.0f, -0.5f, 0.0f };
  ```

### `App/GameObject/Character/Player/PlayerWeapon.cpp`

**Initialize():** ParticleManager 関連のコードを削除し `WeaponTrail::Initialize()` に置き換え。

**Update():** 毎フレーム刃先と根本のワールド座標を計算し、攻撃中なら `AddPoint()` を呼ぶ。

```cpp
const Matrix4x4& worldMat = GetWorldTransform()->GetMatWorld();
Vector3 worldTip  = Transform(tipOffset_,  worldMat);
Vector3 worldHilt = Transform(hiltOffset_, worldMat);

if (player_->IsAttack()) {
    trail_->AddPoint(worldTip, worldHilt);
}
trail_->Update(deltaTime);
```

**DrawEffect():** `trail_->Draw()` を呼ぶように変更。

### `App/GameObject/Character/Player/Player.cpp`

`DrawEffect()` から `weapon_->DrawEffect()` を呼ぶよう追加。

```cpp
void Player::DrawEffect() {
    combat_->Draw();
    weapon_->DrawEffect();  // ← 追加
}
```

---

## 描画フロー

```
GameScene::Update()
  └─ player_->Update()
       └─ weapon_->Update()
            ├─ Transform(tipOffset,  worldMat) → AddPoint(tip, hilt)
            └─ trail_->Update(dt)  ← age 更新・削除

GameScene::Draw() → player_->DrawEffect()
  ├─ combat_->Draw()
  └─ weapon_->DrawEffect()
       └─ trail_->Draw()
            ├─ BuildMesh()     ← CPU で頂点バッファ書き換え
            ├─ SetPipelineState(TrailPSO)
            ├─ SetGraphicsRootConstantBufferView(0, cbAddr)  ← viewProj + tintColor
            ├─ SetGraphicsRootDescriptorTable(1, texSrvIdx)  ← テクスチャ
            └─ DrawInstanced(vertexCount, 1, 0, 0)
```

---

## 調整パラメータ

| パラメータ | 場所 | デフォルト | 説明 |
|-----------|------|-----------|------|
| `tipOffset_` | `PlayerWeapon.h` | `{0, 0.5, 0}` | 刃先のローカルオフセット。武器モデルに合わせて調整 |
| `hiltOffset_` | `PlayerWeapon.h` | `{0, -0.5, 0}` | 根本のローカルオフセット。武器モデルに合わせて調整 |
| `lifetime_` | `WeaponTrail.h` | `0.25f` (秒) | 軌跡の残存時間 |
| `tintColor_` | `WeaponTrail.h` | `{0.5, 0.85, 1.0, 1.0}` | 青白系の色合い |
| `kMaxPoints` | `WeaponTrail.h` | `32` | 最大サンプル点数（多いほど軌跡が長い） |

> **tipOffset / hiltOffset の調整方法**  
> 実行中に ImGui で `defaultPosition_` / `defaultRotation_` を動かしながら刃の向きを確認し、  
> 刃先端と根本にあたる方向に合わせて値を調整する。
