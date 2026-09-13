#pragma once
#include "PlayerStateBase.h"

/// <summary>
/// プレイヤーが敵の攻撃を受けたときの被弾リアクション（仕様書 §11・§12）。
///
/// **速度はこのステートが持たない**。ノックバックの速度・減衰・時間は Player が持つ
/// KnockbackComponent（敵と同じ部品）の担当で、ここは操作不能時間だけを見る。
///
/// 仕様書 §12・§23 ⑦にあるとおり、プレイヤーの拘束は必要以上に長引かせない。
/// 軽被弾はすぐ操作へ戻し、強被弾だけ着地または上限時間まで待つ。
/// </summary>
class PlayerStateKnockBack : public PlayerStateBase
{
public:
    void Enter(Player& player) override;
    void Update(Player& player, float deltaTime) override;
    void Exit(Player& player) override;
    void ExecuteCommand(Player&, const PlayerCommand&) override {}
    const char* GetDebugName() const override { return "Knockback"; }

private:
    // 操作不能時間の上限[秒]。これを過ぎたら空中でも操作を返す
    static constexpr float kLightMaxDuration = 0.25f;
    static constexpr float kHeavyMaxDuration = 0.6f;
    // 着地していても、これより早くは戻さない（当たった瞬間に動けると被弾が伝わらない）
    static constexpr float kMinDuration = 0.1f;

    float timer_ = 0.0f;
    float maxDuration_ = kLightMaxDuration;
};
