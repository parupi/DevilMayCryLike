#include "BossStateApproach.h"
#include "BossStateCombatIdle.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Player/Player.h"

namespace {
	// 地面の上でのプレイヤーまでの距離[m]。プレイヤーが居なければ 0（＝もう着いている扱い）
	float HorizontalDistanceToPlayer(Enemy& enemy) {
		Player* player = enemy.GetPlayer();
		if (!player) return 0.0f;
		Vector3 toPlayer = player->GetWorldTransform()->GetTranslation() - enemy.GetWorldTransform()->GetTranslation();
		toPlayer.y = 0.0f;
		return Length(toPlayer);
	}
}

BossStateApproach::BossStateApproach(EnemyMovementComponent* movement)
	: movement_(movement) {}

void BossStateApproach::Enter(Enemy&) { timer_ = 0.0f; }

void BossStateApproach::Update(Enemy& enemy, float deltaTime) {
	timer_ += deltaTime;

	// 近距離の間合いまで詰める。間合いは配置スケールで広がるので、
	// どの大きさで置いても噛みつき・叩きつけが届く所で止まる
	const float stopDistance = BossStateCombatIdle::GetCloseRange(enemy);
	movement_->MoveToward(enemy, kSpeed, stopDistance);

	// 着いたら残り時間を待たずに次の行動を選び直す
	// （止まったまま時間を使い切ると、目の前で棒立ちになる）
	if (timer_ >= kDuration || HorizontalDistanceToPlayer(enemy) <= stopDistance) {
		movement_->Stop(enemy);
		enemy.ChangeState(BossStateName::CombatIdle);
	}
}

void BossStateApproach::Exit(Enemy& enemy) {
	movement_->Stop(enemy);
}
