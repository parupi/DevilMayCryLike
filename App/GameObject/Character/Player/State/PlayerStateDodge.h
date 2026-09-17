#pragma once
#include "PlayerStateBase.h"

/// <summary>
/// 回避ステート。
///
/// RightTrigger で入り、決めた方向へ一定速度で滑る。
/// この間は無敵で、敵の攻撃に当たるとダメージではなくジャスト回避になる。
/// 回避時間を使い切ったら、止まらずにそのままダッシュへ発展する（仕様書 §18.1）。
/// </summary>
class PlayerStateDodge : public PlayerStateBase
{
public:
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "Dodge"; }
};
