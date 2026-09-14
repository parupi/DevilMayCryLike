#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector2.h>

/// <summary>
/// 熱の歪み（陽炎）。画面上の「帯」の範囲だけ、ノイズで絵を揺らす。
///
/// 帯は始点と終点（UV）と、それぞれの太さで決める。炎のブレスなら口が始点・炎の先が終点になる。
/// 始点と終点を同じにすれば円（溜めの間の口元など）。
/// ノイズはシェーダ内で作るのでテクスチャは要らない。
///
/// 簡易版なので、帯の手前にあるもの（プレイヤーなど）も一緒に揺れる。
/// 既定では無効状態で生成される。
/// </summary>
class HeatDistortionEffect : public BaseOffScreen
{
public:
    explicit HeatDistortionEffect(const std::string& name);
    ~HeatDistortionEffect();

    void Update() override;
    void Draw()   override;

    // HLSL の cbuffer と一致させること（48バイト）
    struct HeatDistortionData {
        Vector2 segmentStart{ 0.5f, 0.5f }; // 帯の始点（UV）
        Vector2 segmentEnd{ 0.5f, 0.5f };   // 帯の終点（UV）
        float radiusStart = 0.05f;          // 始点の太さ（画面の縦を1とした長さ）
        float radiusEnd = 0.1f;             // 終点の太さ
        float strength = 0.0f;              // UVをずらす量の最大（0.0で無効）
        float time = 0.0f;                  // 模様を動かす経過時間[s]（Update が進める）
        float aspect = 16.0f / 9.0f;        // 画面の横/縦
        float noiseScale = 18.0f;           // 模様の細かさ
        float scrollSpeed = 1.5f;           // 模様が昇っていく速さ
        float _pad0 = 0.0f;
    };

    HeatDistortionData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    HeatDistortionData  effectData_;
    HeatDistortionData* effectDataPtr_ = nullptr;
};
