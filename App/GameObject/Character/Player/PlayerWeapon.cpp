#include "PlayerWeapon.h"
#include "World3D/Collider/OBBCollider.h"
#include "Player.h"
#include "Scene/Transition/TransitionManager.h"
#include "Math/MathUtils.h"

namespace {
	// 攻撃名からチュートリアルの種類を判定する（該当しない攻撃はCountを返す）
	TutorialState ResolveTutorialState(const std::string& attackName) {
		if (attackName.rfind("AttackComboA", 0) == 0) return TutorialState::AttackA;
		if (attackName.rfind("AttackComboB", 0) == 0) return TutorialState::AttackB;
		if (attackName == "AttackHighTime") return TutorialState::RoundUpAttack;
		return TutorialState::Count;
	}
}

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName) {}

void PlayerWeapon::Initialize() {
	Object3d::Initialize();

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {0.5f, 1.0f, 0.5f};

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<OBBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().halfExtents = {0.5f, 1.0f, 0.5f};

	trail_ = std::make_unique<WeaponTrail>();
	trail_->Initialize();

	defaultPosition_ = {0.0f, 0.1f, -0.5f};
	defaultRotation_ = {0.0f, 90.0f, 150.0f};

	GetWorldTransform()->GetTranslation() = defaultPosition_;
	GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
}

void PlayerWeapon::Update(float deltaTime) {
	if (!player_) return;

	// 攻撃中ならエフェクトを発生させる
	static_cast<OBBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().isActive = isAttack_;
	// 刃先・根本のワールド座標を計算してトレイルに渡す
	const Matrix4x4& worldMat = GetWorldTransform()->GetMatWorld();
	Vector3 worldTip = Transform(tipOffset_, worldMat);
	Vector3 worldHilt = Transform(hiltOffset_, worldMat);

	if (player_->IsAttack()) {
		trail_->AddPoint(worldTip, worldHilt);
	}
	trail_->Update(deltaTime);

	Object3d::Update(deltaTime);
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
		// 攻撃がヒットしたのでプレイヤーのライトを強く光らせる
		player_->GetCharacterLight()->Flash();

		// チュートリアル対象の攻撃であれば進行させる
		TutorialState tutorialState = ResolveTutorialState(player_->GetCombat()->GetCurrentAttackName());
		if (tutorialState != TutorialState::Count) {
			player_->GetTutorialService()->StepTutorial(tutorialState);
		}
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

