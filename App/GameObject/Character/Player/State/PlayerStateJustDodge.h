#pragma once
#include "PlayerStateBase.h"

/// <summary>
/// ジャスト回避ステート。
///
/// 回避中に敵の攻撃判定へ触れると入る。ダメージは0のまま、
/// 短いスローモーション・白フラッシュ・衝撃波・専用SEで「避けた」ことを伝える。
/// 演出が終わったら、回避が残っていれば回避へ、使い切っていればダッシュへ抜ける。
/// 演出中も回避の移動は続けるので、動きが途切れて見えない（仕様書 §8.2）。
/// </summary>
class PlayerStateJustDodge : public PlayerStateBase
{
public:
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "JustDodge"; }
};
