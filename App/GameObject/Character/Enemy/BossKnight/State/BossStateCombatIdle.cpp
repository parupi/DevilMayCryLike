#include "BossStateCombatIdle.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include <cstdlib>

BossStateCombatIdle::BossStateCombatIdle(EnemySensorComponent* sensor, EnemyMovementComponent* movement,
	float maxHp, BossBattleMemory* memory)
	: sensor_(sensor), movement_(movement), maxHp_(maxHp), memory_(memory) {}

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

// 必殺技のブレスを撃つかどうか。距離・フェーズとは別枠で判定する。
//   1. HPが半分を切った瞬間に解禁し、その1回目は必ず撃つ（「半分でブレスが来る」を保証）
//   2. 以降は kBreathChance の確率で混ぜる
// 解禁フラグはボス本体が持つ BossBattleMemory に記録するので、
// Idle / Move / CombatIdle のどのインスタンスから来ても同じ記憶を見る
bool BossStateCombatIdle::ShouldUseBreath(const Enemy& enemy, int roll) {
	if (!memory_ || maxHp_ <= 0.0f) return false;

	if (!memory_->breathUnlocked && (enemy.GetHp() / maxHp_) <= kBreathHpRatio) {
		memory_->breathUnlocked = true;
		memory_->breathUsed = false;
	}
	if (!memory_->breathUnlocked) return false;

	return !memory_->breathUsed || (roll < kBreathChance);
}

void BossStateCombatIdle::Update(Enemy& enemy, float deltaTime) {
	if (!enemy.GetOnGround()) return;

	sensor_->Update(enemy);
	cooldown_ -= deltaTime;
	if (cooldown_ > 0.0f) return;

	int   phase = GetPhase(enemy.GetHp());
	float dist = sensor_->GetDistanceToPlayer();
	int   roll = std::rand() % 100;

	// 攻撃行動が許可されていない敵（トレーニングの攻撃抑制など）は、
	// 攻撃の抽選結果を接近に置き換える
	const bool canAttack = enemy.CanAttack();
	auto attack = [canAttack](const char* attackState) {
		return canAttack ? attackState : BossStateName::Approach;
	};

	// ─── 必殺技: 火炎ブレス ───────────────────────────────────────
	// 距離を問わず割り込む（リーチが11mあるので中〜遠距離からでも届く）
	if (canAttack && ShouldUseBreath(enemy, roll)) {
		memory_->breathUsed = true;
		enemy.ChangeState(BossStateName::Breath);
		cooldown_ = GetCooldown(phase);
		return;
	}

	// ─── フェーズ別・距離別の行動選択 ─────────────────────────────
	if (phase == 1) {
		if (dist < 4.0f) {
			// 近距離: 通常斬り主体
			if (roll < 60) enemy.ChangeState(attack(BossStateName::Slash));
			else if (roll < 90) enemy.ChangeState(attack(BossStateName::HeavySword));
			else enemy.ChangeState(BossStateName::Approach);
		}
		else if (dist < 12.0f) {
			// 中距離: 接近か斬り
			if (roll < 50) enemy.ChangeState(BossStateName::Approach);
			else if (roll < 80) enemy.ChangeState(attack(BossStateName::Slash));
			else enemy.ChangeState(attack(BossStateName::Rush));
		}
		else {
			// 遠距離: 接近優先
			if (roll < 80) enemy.ChangeState(BossStateName::Approach);
			else enemy.ChangeState(attack(BossStateName::Rush));
		}
	}
	else if (phase == 2) {
		if (dist < 4.0f) {
			// 近距離: 斬り・叩きつけを均等に
			if (roll < 50) enemy.ChangeState(attack(BossStateName::Slash));
			else enemy.ChangeState(attack(BossStateName::HeavySword));
		}
		else if (dist < 12.0f) {
			// 中距離: 突進が増える
			if (roll < 45) enemy.ChangeState(attack(BossStateName::Rush));
			else if (roll < 75) enemy.ChangeState(attack(BossStateName::Slash));
			else enemy.ChangeState(BossStateName::Approach);
		}
		else {
			if (roll < 65) enemy.ChangeState(attack(BossStateName::Rush));
			else enemy.ChangeState(BossStateName::Approach);
		}
	}
	else {
		// フェーズ3: 最大攻撃的
		if (dist < 4.0f) {
			if (roll < 40) enemy.ChangeState(attack(BossStateName::Slash));
			else enemy.ChangeState(attack(BossStateName::HeavySword));
		}
		else if (dist < 12.0f) {
			if (roll < 65) enemy.ChangeState(attack(BossStateName::Rush));
			else enemy.ChangeState(attack(BossStateName::HeavySword));
		}
		else {
			if (roll < 85) enemy.ChangeState(attack(BossStateName::Rush));
			else enemy.ChangeState(BossStateName::Approach);
		}
	}

	cooldown_ = GetCooldown(phase);
}
