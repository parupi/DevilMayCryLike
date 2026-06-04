#pragma once
#include "BaseOffScreen.h"

class VignetteEffect : public BaseOffScreen
{
public:
    VignetteEffect(const std::string& name);
    ~VignetteEffect();

    void Update() override;
    void Draw()   override;

    // ----- パラメータ構造体 -----
    // HLSL cbuffer との完全一致が必要なためパディングを明示する
    // Layout (32 bytes, 2 rows of 16):
    //   offset  0: radius
    //   offset  4: intensity
    //   offset  8: softness
    //   offset 12: _pad0
    //   offset 16: colorR  (vignetteColor.x)
    //   offset 20: colorG  (vignetteColor.y)
    //   offset 24: colorB  (vignetteColor.z)
    //   offset 28: _pad1   (vignetteColor.w unused)
    struct VignetteEffectData {
        float radius    = 0.5f;
        float intensity = 0.5f;
        float softness  = -0.1f;
        float _pad0     = 0.0f;
        float colorR    = 0.0f;  // エッジの色 (デフォルト黒=従来動作)
        float colorG    = 0.0f;
        float colorB    = 0.0f;
        float _pad1     = 0.0f;
    };

    VignetteEffectData& GetEffectData() { return effectData_; }

    void SetColor(float r, float g, float b) {
        effectData_.colorR = r;
        effectData_.colorG = g;
        effectData_.colorB = b;
    }

    void SetActive(bool flag) { isActive_ = flag; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    VignetteEffectData  effectData_;
    VignetteEffectData* effectDataPtr_ = nullptr;
};
