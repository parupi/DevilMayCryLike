#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector3.h>

/// <summary>
/// 画面全体の色の調整（カラーグレーディング）。
/// コントラスト → ゲイン（乗算）・リフト（加算）→ 彩度 の順にかけ、strength で元の絵と混ぜる。
/// 炎のブレス中だけ暖色に寄せる、といった一時的な画作りに使う。既定では無効状態で生成される。
/// </summary>
class ColorGradingEffect : public BaseOffScreen
{
public:
    explicit ColorGradingEffect(const std::string& name);
    ~ColorGradingEffect();

    void Update() override;
    void Draw()   override;

    // HLSL の cbuffer と一致させること（48バイト）
    struct ColorGradingData {
        Vector3 gain{ 1.0f, 1.0f, 1.0f }; // 色ごとの乗算（暖色なら R を上げて B を下げる）
        float strength = 0.0f;            // 元の絵と混ぜる割合（0.0で無効）
        Vector3 lift{ 0.0f, 0.0f, 0.0f }; // 色ごとの加算（暗部の色味）
        float saturation = 1.0f;          // 彩度（1=そのまま）
        float contrast = 1.0f;            // コントラスト（1=そのまま）
        float _pad0 = 0.0f;
        float _pad1 = 0.0f;
        float _pad2 = 0.0f;
    };

    ColorGradingData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    ColorGradingData  effectData_;
    ColorGradingData* effectDataPtr_ = nullptr;
};
