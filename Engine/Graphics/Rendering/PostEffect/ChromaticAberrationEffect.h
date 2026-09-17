#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector2.h>

/// RGBを中心からの方向にずらす色収差。
/// ヒットの瞬間に一瞬かけたり、高ランク時に薄く常時かけたりする想定。
class ChromaticAberrationEffect : public BaseOffScreen
{
public:
    explicit ChromaticAberrationEffect(const std::string& name);
    ~ChromaticAberrationEffect();

    void Update() override;
    void Draw()   override;

    // HLSL の cbuffer と一致させること（16バイト）
    struct ChromaticAberrationData {
        Vector2 center{ 0.5f, 0.5f }; // ずらしの中心（UV）
        float   strength = 0.0f;      // 0.0で無効
        float   _pad0 = 0.0f;
    };

    ChromaticAberrationData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    ChromaticAberrationData  effectData_;
    ChromaticAberrationData* effectDataPtr_ = nullptr;
};
