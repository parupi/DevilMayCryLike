#pragma once
#include "PlayerStateBase.h"

/// <summary>
/// ダッシュステート。
///
/// 回避から自動で発展してくる高速移動。回避より速く、スティックで方向を曲げられる。
/// 入力を離す・時間切れ・ジャンプ・攻撃で終わる。
/// </summary>
class PlayerStateDash : public PlayerStateBase
{
public:
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "Dash"; }

private:
	// ダッシュ開始からの経過時間
	float elapsed_ = 0.0f;
	// 移動入力が無い状態が続いている時間。dashInputGrace を超えたら終了
	float noInputTime_ = 0.0f;
};
