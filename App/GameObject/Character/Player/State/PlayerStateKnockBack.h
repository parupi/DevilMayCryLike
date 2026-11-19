#pragma once
#include "PlayerStateBase.h"

/// <summary>
/// プレイヤーが敵の攻撃を受けたときの被弾リアクション
/// </summary>
class PlayerStateKnockBack : public PlayerStateBase {
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
