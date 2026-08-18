#pragma once
#include <string>
#include <memory>

class Enemy;

class EnemyStateBase {
public:
	virtual ~EnemyStateBase() = default;
	virtual void Enter(Enemy& enemy) { enemy; }
	virtual void Update(Enemy& enemy, float deltaTime) = 0;
	virtual void Exit(Enemy& enemy) { enemy; }

	/// <summary>
	/// 意思決定ではなく「被弾・落下への反応」を再生しているステートか。
	///
	/// Enemy::SetActionSuppressed()（トレーニングの棒立ちモード）は意思決定だけを止めたいので、
	/// これが true のステートは止めずに最後まで再生させる。
	/// 止めてしまうと、のけぞりの傾きが戻らない・空中で固まる、といった見た目の破綻になる
	/// </summary>
	virtual bool IsReaction() const { return false; }
};