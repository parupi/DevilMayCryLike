#include "PlayerWeapon.h"
#include <3d/Collider/AABBCollider.h>
#include "Player.h"
#include <scene/Transition/TransitionManager.h>

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName)
{
}

void PlayerWeapon::Initialize()
{
	Object3d::Initialize();

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {0.5f, 0.5f, 0.5f };

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMax = { 0.5f, 0.5f, 0.5f };
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMin = { -0.5f, -0.5f, -0.5f };

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize("smoke");
	smokeEmitter_->SetParticle("smoke");
}

void PlayerWeapon::Update(float deltaTime)
{
	if (player_ == nullptr) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
	}

	// カメラ合切り替え中とフェード中は動かさない
	if (!TransitionManager::GetInstance()->IsFinished() || CameraManager::GetInstance()->IsTransition()) return;

	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().isActive = isAttack_;
	smokeEmitter_->Update();

	
	Object3d::Update(deltaTime);
}

void PlayerWeapon::Draw()
{
	if (isAttack_) {
		Object3d::Draw();
	}
}

void PlayerWeapon::DrawEffect()
{
}

void PlayerWeapon::OnCollisionEnter(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Enemy) {
		scoreManager_->AddScore(50);
		// 攻撃のパラメータを参照してヒットストップ起動
		player_->GetHitStop()->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity);
	}

	if (other->category_ == CollisionCategory::Ground) {
		smokeEmitter_->Emit();
	}
}

void PlayerWeapon::OnCollisionStay(BaseCollider* other)
{
	other;
}

void PlayerWeapon::OnCollisionExit(BaseCollider* other)
{
	other;
}

#ifdef _DEBUG
void PlayerWeapon::DebugGui()
{
	Object3d::DebugGui();

}

#endif // _DEBUG

