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

    // HLSL の cbuffer と一致させること（80バイト）
    struct ColorGradingData {
        Vector3 gain{ 1.0f, 1.0f, 1.0f }; // 色ごとの乗算（暖色なら R を上げて B を下げる）
        float strength = 0.0f;            // 元の絵と混ぜる割合（0.0で無効）
        Vector3 lift{ 0.0f, 0.0f, 0.0f }; // 色ごとの加算（暗部の色味）
        float saturation = 1.0f;          // 彩度（1=そのまま）
        float contrast = 1.0f;            // コントラスト（1=そのまま）
        // 1 なら見た目の明るさ（ガンマ）の空間で計算する。0 はリニアのまま（ブレス用はこちらで調整済み）。
        // 暗い絵ではリニアのままだとコントラストの中心 0.5 が明るすぎて、暗部がまとめて潰れる
        float perceptual = 0.0f;
        float splitBalance = 0.5f;        // 暗部とハイライトの境目の明るさ
        float contrastPivot = 0.5f;       // コントラストの中心。暗い絵では下げないと暗部が潰れる
        Vector3 shadowTint{ 0.0f, 0.0f, 0.0f };   // 暗部に足す色（青緑にすると洞窟らしい）
        float shadowTintStrength = 0.0f;          // 0 で無効
        Vector3 highlightTint{ 1.0f, 1.0f, 1.0f }; // ハイライトに掛ける色（暖色にすると松明の色が残る）
        float highlightTintStrength = 0.0f;       // 0 で無効
    };

    ColorGradingData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    ColorGradingData  effectData_;
    ColorGradingData* effectDataPtr_ = nullptr;
};
