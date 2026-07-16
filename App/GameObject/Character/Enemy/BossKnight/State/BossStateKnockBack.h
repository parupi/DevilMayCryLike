#pragma once
#include "GameObject/Character/Enemy/State/EnemyStateKnockBack.h"

/// <summary>
/// ボス専用の吹き飛びステート。
/// 挙動は共通の EnemyStateKnockBack と同じ（飛んで着地する）が、
/// 着地後は Idle ではなくダッシュ攻撃(Rush)へ即座に移行する。
/// </summary>
class BossStateKnockBack : public EnemyStateKnockBack
{
protected:
	// 着地後の遷移先を Rush にする。
	const char* NextState() const override;
};
