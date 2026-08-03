#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector4.h>

/// 画面全体を一瞬光らせる加算フラッシュ。
class HitFlashEffect : public BaseOffScreen
{
public:
    explicit HitFlashEffect(const std::string& name);
    ~HitFlashEffect();

    void Update() override;
    void Draw()   override;

    // HLSL の cbuffer と一致させること（16バイト）
    struct HitFlashData {
        // rgb = 光の色, w = 強さ（0.0で無効）
        Vector4 flashColor{ 1.0f, 1.0f, 1.0f, 0.0f };
    };

    HitFlashData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    HitFlashData  effectData_;
    HitFlashData* effectDataPtr_ = nullptr;
};
