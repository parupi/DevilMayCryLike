#include "BossStateCombatIdle.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include <cstdlib>

BossStateCombatIdle::BossStateCombatIdle(EnemySensorComponent* sensor, EnemyMovementComponent* movement,
	float maxHp)
	: sensor_(sensor), movement_(movement), maxHp_(maxHp) {}

// 次の行動を選ぶまでの間を置く。
// **ここを 0 にしてはいけない**。攻撃ステートは終わると必ずこのステートへ戻ってくるので、
// 0 にすると戻った次のフレームにまた攻撃を選び、間が一切空かない
// （Update 末尾の cooldown_ = GetCooldown() は Exit → Enter で毎回上書きされるため効かなかった）
void BossStateCombatIdle::Enter(Enemy& enemy) {
	cooldown_ = GetCooldown(GetPhase(enemy.GetHp()));
}
void BossStateCombatIdle::Exit(Enemy&) {}

int BossStateCombatIdle::GetPhase(float hp) const {
	float ratio = hp / maxHp_;
	if (ratio > 0.66f) return 1;
	if (ratio > 0.33f) return 2;
	return 3;
}

// 攻撃と攻撃の間の「溜め」。攻撃モーション自体の長さ（噛みつき0.88秒・叩きつけ1.67秒）に
// これが上乗せされるので、1.2秒ならフェーズ1の攻撃周期はおよそ2秒になる。
// 攻撃頻度を変えたいときはこの3つを触る（大きくするほど攻撃が減る）
float BossStateCombatIdle::GetCooldown(int phase) const {
	if (phase == 1) return 1.2f;
	if (phase == 2) return 0.8f;
	return 0.5f; // フェーズ3: 素早く判断
}

void BossStateCombatIdle::Update(Enemy& enemy, float deltaTime) {
	if (!enemy.GetOnGround()) return;

	sensor_->Update(enemy);
	cooldown_ -= deltaTime;
	if (cooldown_ > 0.0f) return;

	int   phase = GetPhase(enemy.GetHp());
	float dist = sensor_->GetDistanceToPlayer();
	int   roll = std::rand() % 100;

	// ─── フェーズ別・距離別の行動選択 ─────────────────────────────
	if (phase == 1) {
		if (dist < 4.0f) {
			// 近距離: 通常斬り主体
			if (roll < 60) enemy.ChangeState(BossStateName::Slash);
			else if (roll < 90) enemy.ChangeState(BossStateName::HeavySword);
			else enemy.ChangeState(BossStateName::Approach);
		}
		else if (dist < 12.0f) {
			// 中距離: 接近か斬り
			if (roll < 50) enemy.ChangeState(BossStateName::Approach);
			else if (roll < 80) enemy.ChangeState(BossStateName::Slash);
			else enemy.ChangeState(BossStateName::Rush);
		}
		else {
			// 遠距離: 接近優先
			if (roll < 80) enemy.ChangeState(BossStateName::Approach);
			else enemy.ChangeState(BossStateName::Rush);
		}
	}
	else if (phase == 2) {
		if (dist < 4.0f) {
			// 近距離: 斬り・叩きつけを均等に
			if (roll < 50) enemy.ChangeState(BossStateName::Slash);
			else enemy.ChangeState(BossStateName::HeavySword);
		}
		else if (dist < 12.0f) {
			// 中距離: 突進が増える
			if (roll < 45) enemy.ChangeState(BossStateName::Rush);
			else if (roll < 75) enemy.ChangeState(BossStateName::Slash);
			else enemy.ChangeState(BossStateName::Approach);
		}
		else {
			if (roll < 65) enemy.ChangeState(BossStateName::Rush);
			else enemy.ChangeState(BossStateName::Approach);
		}
	}
	else {
		// フェーズ3: 最大攻撃的
		if (dist < 4.0f) {
			if (roll < 40) enemy.ChangeState(BossStateName::Slash);
			else enemy.ChangeState(BossStateName::HeavySword);
		}
		else if (dist < 12.0f) {
			if (roll < 65) enemy.ChangeState(BossStateName::Rush);
			else enemy.ChangeState(BossStateName::HeavySword);
		}
		else {
			if (roll < 85) enemy.ChangeState(BossStateName::Rush);
			else enemy.ChangeState(BossStateName::Approach);
		}
	}

	cooldown_ = GetCooldown(phase);
}
