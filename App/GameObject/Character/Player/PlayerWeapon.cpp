#include "PlayerWeapon.h"
#include "3d/Collider/AABBCollider.h"
#include "Player.h"
#include "scene/Transition/TransitionManager.h"
#include "base/Particle/ParticleManager.h"

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName) {}

void PlayerWeapon::Initialize() {
	Object3d::Initialize();

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = { 0.5f, 0.5f, 0.5f };

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMax = { 0.5f, 0.5f, 0.5f };
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMin = { -0.5f, -0.5f, -0.5f };

	ParticleManager::GetInstance()->CreateParticleGroup("WeaponTrailEffect", "circle.png");
	ParticleManager::GetInstance()->CreateEmitter("PlayerWeaponTrailEmitter", "WeaponTrailEmitter");
	auto& emitters = ParticleManager::GetInstance()->GetEmitters();
	emitter_ = emitters.at("PlayerWeaponTrailEmitter").get();
	emitter_->SetParent(GetWorldTransform());

	defaultPosition_ = { 0.0f, 0.6f, -0.5f };
	defaultRotation_ = { 0.0f, 90.0f, 150.0f };

	//GetWorldTransform()->GetTranslation() = defaultPosition_;
	//GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
}

void PlayerWeapon::Update(float deltaTime) {

	// オブジェクトマネージャからプレイヤーを探して生ポインタを受け取る
	if (player_ == nullptr) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
	}

	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().isActive = isAttack_;
	if (player_->IsAttack()) {
		emitter_->Emit();
	}

	//ImGui::Begin("Weapon Debug");
	//ImGui::DragFloat3("Position", &defaultPosition_.x, 0.1f);
	//ImGui::DragFloat3("Rotation", &defaultRotation_.x, 0.1f);
	//ImGui::End();

	//GetWorldTransform()->GetTranslation() = defaultPosition_;
	//GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);

	// 座標が全て0の時に止める	
	if (GetWorldTransform()->GetTranslation() == Vector3{ 0.0f, 0.0f, 0.0f }) {
		GetWorldTransform()->GetTranslation() = defaultPosition_;
		GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
	}

	Object3d::Update(deltaTime);
}

void PlayerWeapon::Draw() {
	Object3d::Draw();
}

void PlayerWeapon::DrawEffect() {}

void PlayerWeapon::OnCollisionEnter(BaseCollider* other) {
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

void PlayerWeapon::OnCollisionStay(BaseCollider* other) {
	other;
}

void PlayerWeapon::OnCollisionExit(BaseCollider* other) {
	other;
}

#ifdef _DEBUG
void PlayerWeapon::DebugGui() {
	Object3d::DebugGui();

}

#endif // _DEBUG

