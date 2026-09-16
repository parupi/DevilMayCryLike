#include "BossStateRoar.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"
#include "GameObject/Camera/GameCamera.h"
#include "World3D/Camera/CameraManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"

BossStateRoar::BossStateRoar(EnemyMovementComponent* movement)
	: movement_(movement) {}

void BossStateRoar::Enter(Enemy& enemy) {
	timer_ = 0.0f;
	burst_ = false;
	movement_->Stop(enemy);
	enemy.SetAttackAnimationSpeed(kWingFlapSpeed);

	// フェーズ移行の咆哮。立ち上がりが遅い音なので、衝撃波（Burst）に向かって膨らむ
	SoundManager::GetInstance().PlaySE3D(
		GameSound::kDragonPhaseRoar, enemy.GetWorldTransform()->GetTranslation(), 1.0f);
}

void BossStateRoar::Update(Enemy& enemy, float deltaTime) {
	timer_ += deltaTime;
	movement_->Stop(enemy);

	if (!burst_ && timer_ >= kBurstTime) {
		burst_ = true;
		Burst(enemy);
	}

	if (timer_ >= kDuration) {
		enemy.ChangeState(BossStateName::CombatIdle);
	}
}

void BossStateRoar::Exit(Enemy& enemy) {
	enemy.ClearAttackAnimationSpeed();
}

void BossStateRoar::Burst(Enemy& enemy) {
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddShake(kShakeTrauma);
	}
	// 足元から横へ広がる衝撃波。上へ向けて立てるとリングが地面に沿って寝る
	ParticleManager::GetInstance().PlayVFX(BossKnight::kRoarVfxName, enemy.GetFootPosition(), { 0.0f, 1.0f, 0.0f });
	enemy.FlashLight();
}
