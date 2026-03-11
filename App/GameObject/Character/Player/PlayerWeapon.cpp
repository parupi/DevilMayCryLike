#include "PlayerWeapon.h"
#include "3d/Collider/AABBCollider.h"
#include "Player.h"
#include "scene/Transition/TransitionManager.h"
#include "base/Particle/ParticleManager.h"

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

	ParticleManager::GetInstance()->CreateParticleGroup("WeaponTrailEffect", "circle.png");
	ParticleManager::GetInstance()->CreateEmitter("PlayerWeaponTrailEmitter", "WeaponTrailEmitter");
	auto& emitters = ParticleManager::GetInstance()->GetEmitters();
	emitter_ = emitters.at("PlayerWeaponTrailEmitter").get();
	emitter_->SetParent(GetWorldTransform());
}

void PlayerWeapon::Update(float deltaTime)
{
	// オブジェクトマネージャからプレイヤーを探して生ポインタを受け取る
	if (player_ == nullptr) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
	}

	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().isActive = isAttack_;
	if (player_->IsAttack()) {
		emitter_->Emit();
	}
	
	
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
		if (!player_->IsAttack()) return;
		
		scoreManager_->AddScore(50);
		// 攻撃のパラメータを参照してヒットストップ起動
		player_->GetHitStop()->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity);
	}

	if (other->category_ == CollisionCategory::Ground) {
		//smokeEmitter_->Emit();
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

