# シャドウマップ品質改善 — 変更まとめ

## 概要

ハードシャドウ（二値）の単一シャドウマップから、PCF ソフトシャドウ付きの **Cascaded Shadow Map (CSM)** へ段階的に改善した。

---

## 変更前の状態

| 項目 | 状態 |
|------|------|
| シャドウタイプ | ハードシャドウ（0 or 1 の二値） |
| カスケード | `kCascadeCount = 3` の構造だが全カスケードが同一行列 |
| `CalculateCascadeSplits()` | 空関数（未実装） |
| `CalculateLightMatrices()` | 固定 `orthoSize` のオルソ投影をコピーするだけ |
| シェーダーサンプリング | `SampleCmpLevelZero` ではなく `Sample`（比較サンプラー未使用） |
| シャドウマップバインド | カスケード 0 の 1 枚のみ |

---

## 変更内容

### 1. PCF（Percentage Closer Filtering）実装

**ファイル:** `Resource/shaders/Lighting.hlsli`

- `gShadow.Sample(sampler_Linear, uv)` による二値判定を廃止
- `SampleCmpLevelZero(sampler_Shadow, ...)` で比較サンプラーを正しく使用
- 3×3 カーネルで 9 サンプルを平均化することでエッジをソフト化

> C++ 側のサンプラー（`D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT` / `D3D12_COMPARISON_FUNC_LESS_EQUAL`）はすでに正しく設定済みだったため変更不要。

---

### 2. カスケード分割計算の実装

**ファイル:** `Engine/Graphics/Rendering/Shadow/CascadedShadowMap.cpp`

`CalculateCascadeSplits()` を実装。**Practical Split Scheme** により対数分割と均等分割をブレンドしてカスケード境界を決定する。

```
splitI = lambda * (near * (far/near)^(i/N)) + (1 - lambda) * (near + (far - near) * i/N)
```

- `splitLambda_ = 0.7f`（対数寄り）で近距離を高解像度・遠距離を低解像度に配分
- 最終カスケードは常に `shadowFar_` を終端とする

---

### 3. フラスタムフィット投影の実装

**ファイル:** `Engine/Graphics/Rendering/Shadow/CascadedShadowMap.cpp`

`CalculateLightMatrices()` を全面書き直し。固定 `orthoSize` をやめ、各カスケードの視錐台に **タイトなオルソ投影** を生成する。

**処理の流れ:**

1. カスケードの `[nearZ, farZ]` に対応する透視投影行列を一時生成
2. その VP 逆行列で NDC 8 頂点（DX12: Z=`[0,1]`）をワールド空間に逆変換
3. 8 頂点の重心をライトの注視点に設定してライトビュー行列を生成
4. ライトビュー空間での AABB を計算 → `MakeOrthographicMatrix` に渡す
5. `minZ` を `shadowDistance_` 分後退させて影の送り主を含める

---

### 4. パラメータ整理

**ファイル:** `Engine/Graphics/Rendering/Shadow/CascadedShadowMap.h`

| 削除 | 追加 |
|------|------|
| `orthoSize_`（固定投影サイズ）| `splitLambda_`（分割ブレンド係数 [0,1]）|
| `shadowNear_`（固定 near）| — |

残存パラメータ:
- `shadowDistance_` — フラスタム重心からライトを離す距離（影送り主カバー）
- `shadowFar_` — シャドウの最大有効距離

---

### 5. 全カスケードのシェーダーバインド

#### C++ 側

**`CascadedShadowMap.h` — 追加した構造体とフィールド:**

```cpp
struct CascadeLightingCB
{
    Matrix4x4 lightViewProj0;   // cascade 0 の Light VP 行列
    Matrix4x4 lightViewProj1;   // cascade 1
    Matrix4x4 lightViewProj2;   // cascade 2
    float     cascadeFar[4];    // xyz = 各カスケードの view-space far 深度
    Matrix4x4 cameraView;       // view-space Z 計算用
};

uint32_t           lightingCBIndex_;
CascadeLightingCB* mappedLightingCB_;
```

**`CascadedShadowMap.cpp` — 追加した処理:**
- `Initialize()`: ライティングパス用 CB バッファ（512 B）を確保
- `Update()`: 毎フレーム全カスケードの VP 行列・分割深度・カメラビュー行列を書き込み
- `BindCascadeCB(uint32_t rootIndex)`: `SetGraphicsRootConstantBufferView` で CB をバインド

**`PSOManager.cpp` — ルートシグネチャ変更:**

```cpp
// Before
descriptorRangesForShadow[0].NumDescriptors = 1;  // t5 のみ

// After
descriptorRangesForShadow[0].NumDescriptors = 3;  // t5, t6, t7
```

> `CreateDSV()` で `srvIndices_[0~2]` が連続確保されているため、テーブル先頭（index 0）を指定するだけで 3 枚全てが対応する。

**`RenderPipeline.cpp` — 呼び出し変更:**

```cpp
// Before
LightManager::GetInstance()->GetCSM()->Bind(5, 0);  // cascade 0 のみ

// After
LightManager::GetInstance()->GetCSM()->BindCascadeCB(5);  // 全カスケードデータ
```

#### シェーダー側

**`Resource/shaders/Lighting.hlsli` — 最終形:**

```hlsl
Texture2D<float> gShadow0 : register(t5);  // cascade 0
Texture2D<float> gShadow1 : register(t6);  // cascade 1
Texture2D<float> gShadow2 : register(t7);  // cascade 2

cbuffer LightMatrixCB : register(b4)
{
    float4x4 LightViewProj0;
    float4x4 LightViewProj1;
    float4x4 LightViewProj2;
    float4   CascadeFar;       // xyz = view-space far 深度
    float4x4 CameraView;
}

// カスケード選択 + PCF サンプリング
float CalcShadow(float3 worldPos)
{
    float viewZ = mul(float4(worldPos, 1.0), CameraView).z;

    if (viewZ < CascadeFar.x)  return SampleShadowPCF(gShadow0, LightViewProj0, worldPos);
    if (viewZ < CascadeFar.y)  return SampleShadowPCF(gShadow1, LightViewProj1, worldPos);
    if (viewZ < CascadeFar.z)  return SampleShadowPCF(gShadow2, LightViewProj2, worldPos);
    return 1.0;
}
```

---

## 変更ファイル一覧

| ファイル | 変更種別 |
|---------|---------|
| `Resource/shaders/Lighting.hlsli` | PCF 実装 → CSM カスケード選択に拡張 |
| `Engine/Graphics/Rendering/Shadow/CascadedShadowMap.h` | パラメータ整理・CB 構造体・新メソッド追加 |
| `Engine/Graphics/Rendering/Shadow/CascadedShadowMap.cpp` | カスケード分割・フラスタムフィット・CB 書き込み実装 |
| `Engine/Graphics/Rendering/PSO/PSOManager.cpp` | シャドウ SRV テーブルを 1 → 3 に拡張 |
| `Engine/Graphics/Rendering/RenderPath/RenderPipeline.cpp` | `Bind(5,0)` → `BindCascadeCB(5)` |

---

## デバッグ UI（ImGui）

`Shadow Map (CSM)` ウィンドウで以下をリアルタイム調整できる。

| パラメータ | 説明 | 範囲 |
|-----------|------|------|
| `Light Distance` | フラスタム重心からライトを離す距離 | 10 〜 500 |
| `Shadow Far` | 最大シャドウ距離 | 1 〜 2000 |
| `Split Lambda` | 対数(1.0) / 均等(0.0) 分割ブレンド | 0.0 〜 1.0 |

分割深度（view-space Z）は各カスケードの実値として表示される。
