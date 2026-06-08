#pragma once
#include "PlayerStateBase.h"

/// <summary>
/// プレイヤーが敵の攻撃を受けたときのノックバックステート。
/// 被ダメージ情報 (DamageInfo) を Player から読み取り、反発速度を与える。
/// </summary>
class PlayerStateKnockBack : public PlayerStateBase
{
public:
    void Enter(Player& player) override;
    void Update(Player& player, float deltaTime) override;
    void Exit(Player& player) override;
    void ExecuteCommand(Player& player, const PlayerCommand& command) override {}
    const char* GetDebugName() const override { return "Knockback"; }

private:
    float timer_    = 0.0f;
    float duration_ = 0.6f;
};
