#include "PlayerWeapon.h"
#include "3d/Collider/OBBCollider.h"
#include "Player.h"
#include "scene/Transition/TransitionManager.h"
#include "math/function.h"

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName) {}

void PlayerWeapon::Initialize() {
	Object3d::Initialize();

	//GetRenderer("PlayerWeapon")->GetWorldTransform()->GetTranslation() = { 0.0f, 0.5f, 0.0f };
	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = { 0.5f, 1.0f, 0.5f };

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<OBBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().halfExtents = { 0.5f, 1.0f, 0.5f };

	trail_ = std::make_unique<WeaponTrail>();
	trail_->Initialize();

	defaultPosition_ = { 0.0f, 0.1f, -0.5f };
	defaultRotation_ = { 0.0f, 90.0f, 150.0f };

	GetWorldTransform()->GetTranslation() = defaultPosition_;
	GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
}

void PlayerWeapon::Update(float deltaTime) {
	if (!player_) return;

	// 攻撃中ならエフェクトを発生させる
	static_cast<OBBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().isActive = isAttack_;
	// 刃先・根本のワールド座標を計算してトレイルに渡す
	const Matrix4x4& worldMat = GetWorldTransform()->GetMatWorld();
	Vector3 worldTip  = Transform(tipOffset_,  worldMat);
	Vector3 worldHilt = Transform(hiltOffset_, worldMat);

	if (player_->IsAttack()) {
		trail_->AddPoint(worldTip, worldHilt);
	}
	trail_->Update(deltaTime);

	Object3d::Update(deltaTime);

	//ImGui::Begin("Weapon Debug");
	//ImGui::DragFloat3("Position", &defaultPosition_.x, 0.1f);
	//ImGui::DragFloat3("Rotation", &defaultRotation_.x, 0.1f);
	//ImGui::End();


	//GetWorldTransform()->GetTranslation() = defaultPosition_;
	//GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
}

void PlayerWeapon::Draw() {
	Object3d::Draw();
}

void PlayerWeapon::DrawEffect() {
	trail_->Draw();
}

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

