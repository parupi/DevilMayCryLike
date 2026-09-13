#include "EnemyStateKnockBack.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"

void EnemyStateKnockBack::Enter(Enemy& enemy) {
	const KnockbackComponent& knockback = enemy.GetKnockback();

	currentTilt_ = 0.0f;
	targetTilt_ = TiltFor(knockback.GetType());
}

float EnemyStateKnockBack::TiltFor(ReactionType type) {
	return (type == ReactionType::HitStun) ? kStunTiltDegree : kBlowTiltDegree;
}

void EnemyStateKnockBack::Update(Enemy& enemy, float deltaTime) {
	const KnockbackComponent& knockback = enemy.GetKnockback();

	// ── 見た目 ──
	// レンダラーの回転を直接書かずに Enemy 経由で渡す。
	// モデルごとの向き補正（SetModelRotationOffset）と合成されるので、
	// 直接書くと吹き飛んだ敵が正面を向き直してしまう。
	if (knockback.GetType() != ReactionType::HitStun) {
		// 吹き飛び・打ち上げはくるくる回す。経過時間に比例させるので回り続ける
		const float rotate = knockback.GetTorque() * knockback.GetElapsed();
		enemy.SetModelReactionRotation(EulerDegree({ rotate, rotate, rotate }));
	}
	else {
		// 追撃でリアクションの種類が変わってもステートは入れ直さないので、傾き先は毎フレーム引き直す
		targetTilt_ = TiltFor(knockback.GetType());
		currentTilt_ = Lerp(currentTilt_, targetTilt_, deltaTime * kTiltFollowRate);
		enemy.SetModelReactionRotation(EulerDegree({ currentTilt_, 0.0f, 0.0f }));
	}

	// ── 復帰の判定（仕様書 §11）──
	if (knockback.GetType() == ReactionType::HitStun) {
		// のけぞりは地上なので、時間が来たらすぐ戻す（拘束を長引かせない）。
		// 追撃を受けると KnockbackComponent 側でのけぞりが延長される
		if (!knockback.IsStunned() && knockback.IsHorizontalFinished()) {
			enemy.ChangeState(NextState());
		}
		return;
	}

	// 吹き飛び・打ち上げは着地して勢いが収まるまで。
	// 着地の処理（落下速度を消して水平を弱める）は Enemy::ResolveGroundCollision が行う
	if (!knockback.IsActive()) {
		enemy.ChangeState(NextState());
	}
}

const char* EnemyStateKnockBack::NextState() const {
	return EnemyStateName::Idle;
}

void EnemyStateKnockBack::Exit(Enemy& enemy) {
	// のけぞり・吹き飛びで付けた傾きをここで必ず戻す。
	// （以前は着地時にしか戻していなかったので、のけぞりで終わると傾いたままだった。
	//   立方体のときは気づけなかったが、人型のモデルでは傾きっぱなしが目に見える）
	enemy.ClearModelReactionRotation();
}
