#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector2.h>

/// 指定した点へ向かって伸びる放射状ブラー。
/// ヒット位置を中心に一瞬だけかける想定なので、既定では無効状態で生成される。
class RadialBlurEffect : public BaseOffScreen
{
public:
    explicit RadialBlurEffect(const std::string& name);
    ~RadialBlurEffect();

    void Update() override;
    void Draw()   override;

    // HLSL の cbuffer と一致させること（16バイト）
    struct RadialBlurData {
        Vector2 center{ 0.5f, 0.5f }; // ブラーの中心（UV）
        float   strength = 0.0f;      // 0.0で無効
        float   _pad0 = 0.0f;
    };

    RadialBlurData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    RadialBlurData  effectData_;
    RadialBlurData* effectDataPtr_ = nullptr;
};
