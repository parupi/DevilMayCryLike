#pragma once
#include "BaseOffScreen.h"

/// <summary>
/// アンチエイリアス（FXAA 3.11 Quality 相当）。OffScreenManager の「最後の枠」に入れる常設のパス（MyGameTitle が登録）。
///
/// AddEffect のチェーンは App 側が後から付け足していくので、ふつうのエフェクトとして入れると
/// ビネットやグレーディングがこの後ろに来てしまう。そのため SetFinalEffect で、チェーンが全部終わった後に1回だけ走らせる。
/// UI はポストエフェクトの後にバックバッファへ直接描くので、文字はぼけない。
/// </summary>
class FxaaEffect : public BaseOffScreen
{
public:
    // HLSL の FxaaParam と並びを合わせること（FXAA.PS.hlsl）
    struct FxaaData {
        float rcpFrameX = 0.0f;
        float rcpFrameY = 0.0f;
        float subpix = 0.75f;
        // FXAA の既定（0.166 / 0.0833）だと洞窟の暗い床・壁の斜めの縁を拾えなかったので、
        // 高品質寄りの値にしてある（タイトルの回廊で ON/OFF を見比べて決めた）
        float edgeThreshold = 0.125f;
        float edgeThresholdMin = 0.0312f;
        float showEdges = 0.0f;
        float _pad0 = 0.0f;
        float _pad1 = 0.0f;
    };

    explicit FxaaEffect(const std::string& name);
    ~FxaaEffect();

    void Update() override;
    void Draw()   override;

    FxaaData& GetEffectData() { return *effectDataPtr_; }

private:
    uint32_t effectHandle_ = 0;
    FxaaData* effectDataPtr_ = nullptr;
};
