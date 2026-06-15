#include "BossStateSlash.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"

namespace {
	MeleeAttackParams MakeSlashParams() {
		MeleeAttackParams p;
		p.windupDuration = 0.45f;
		p.attackDuration = 0.22f;
		p.rushSpeed = 2.5f;   // 振りながら少し前進

		// 武器を頭上に引いてから一気に振り下ろす
		p.weaponTranslate = {
			{ -1.2f,  1.8f,  0.2f },
			{ -1.2f,  1.8f,  0.2f },
			{  0.3f, -0.1f, -1.2f },
			{  0.4f, -0.3f, -1.2f },
		};
		p.weaponRotate = {
			{  60.0f, -30.0f,  90.0f },
			{  60.0f, -30.0f,  90.0f },
			{ -90.0f,   0.0f,  20.0f },
			{-130.0f,   0.0f,  20.0f },
		};
		return p;
	}
}

BossStateSlash::BossStateSlash(EnemyMeleeAttackComponent* attack)
	: attack_(attack) {}

void BossStateSlash::Enter(Enemy& enemy) {
	attack_->BeginAttack(enemy, MakeSlashParams());
}

void BossStateSlash::Update(Enemy& enemy, float deltaTime) {
	attack_->Update(enemy, deltaTime);
	if (attack_->IsFinished()) {
		enemy.ChangeState(BossStateName::CombatIdle);
	}
}

void BossStateSlash::Exit(Enemy&) {}
