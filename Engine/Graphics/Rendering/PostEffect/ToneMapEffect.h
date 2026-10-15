#pragma once
#include "BaseOffScreen.h"

/// <summary>
/// 露出とトーンマップ（HDR → 0〜1）。ブルームの直後に置く常設のパス（MyGameTitle が登録）。
///
/// ポストエフェクトのバッファは R16F で 1 を超える明るさを持つ。ブルームはその HDR のまま光を抜き出し、
/// ここで 0〜1 に収めてから、グレーディングやビネットなど 0〜1 前提の演出へ渡す（仕様書 3.2 の順番）。
/// 有効・無効と値は毎フレーム RenderEnvironment（toneMapEnabled / exposure / toneMapShoulderStart）から読む。
/// 無効のシーンでは、ライティング結果を写すときに 0〜1 に切り詰めるので、従来と同じ見た目になる。
/// </summary>
class ToneMapEffect : public BaseOffScreen
{
public:
    explicit ToneMapEffect(const std::string& name);
    ~ToneMapEffect();

    void Update() override;
    void Draw()   override;

private:
    // HLSL の ToneMapParam と並びを合わせること（ToneMap.PS.hlsl）
    struct ToneMapData {
        float exposure = 1.0f;
        float shoulderStart = 0.6f;
        float _pad0 = 0.0f;
        float _pad1 = 0.0f;
    };

    uint32_t effectHandle_ = 0;
    ToneMapData* effectDataPtr_ = nullptr;
};
