#include "PlayerWeapon.h"
#include "World3D/Collider/OBBCollider.h"
#include "Player.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "Scene/Transition/TransitionManager.h"
#include "Math/MathUtils.h"
#include "GameObject/Effect/HitEffectSystem.h"

namespace {
	// 攻撃名からチュートリアルの種類を判定する（該当しない攻撃はCountを返す）
	TutorialState ResolveTutorialState(const std::string& attackName) {
		if (attackName.rfind("AttackComboA", 0) == 0) return TutorialState::AttackA;
		if (attackName.rfind("AttackComboB", 0) == 0) return TutorialState::AttackB;
		if (attackName == "AttackHighTime") return TutorialState::RoundUpAttack;
		return TutorialState::Count;
	}

	// 当たった相手のワールド座標。取れない場合は fallback を返す。
	Vector3 CalcHitPosition(BaseCollider* other, const Vector3& fallback) {
		if (!other || !other->owner_) return fallback;
		return other->owner_->GetWorldTransform()->GetTranslation();
	}

	// 攻撃者から被弾者へ向かう方向。重なっている場合は上向きに逃がす。
	Vector3 CalcHitDirection(const Vector3& hitPosition, const Vector3& attackerPosition) {
		const Vector3 toHit = hitPosition - attackerPosition;
		if (Length(toHit) < 0.0001f) return Vector3{ 0.0f, 1.0f, 0.0f };
		return Normalize(toHit);
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

		// 攻撃の状況（空中・強攻撃・敵の強さ）を集めてスタイルスコアへ渡す
		const AttackData& attack = player_->GetAttackData();
		AttackHitContext hitCtx;
		hitCtx.attackName = player_->GetCombat()->GetCurrentAttackName();
		if (hitCtx.attackName.empty()) hitCtx.attackName = attack.name;
		hitCtx.isAir = (attack.posture == AttackPosture::Air);
		// 打ち上げ・吹き飛ばしは強攻撃として高めに評価する
		hitCtx.isStrong = (attack.type == ReactionType::Launch || attack.type == ReactionType::Knockback);
		auto* enemy = dynamic_cast<Enemy*>(other->owner_);
		if (enemy) {
			hitCtx.enemyMultiplier = enemy->GetStyleMultiplier();
		}
		scoreManager_->OnAttackHit(hitCtx);

		// ヒット演出（VFX・カメラシェイク・ポストエフェクト・ヒットストップ・ライト）は
		// HitEffectSystem がまとめて面倒を見る
		HitEffectRequest fx;
		fx.position = CalcHitPosition(other, GetWorldTransform()->GetTranslation());
		fx.direction = CalcHitDirection(fx.position, player_->GetWorldTransform()->GetTranslation());
		fx.strength = attack.hitStopStrength;
		fx.hitStopTime = attack.hitStopTime;
		fx.hitStopIntensity = attack.hitStopIntensity;
		// スーパーアーマーで弾かれた場合は控えめな演出にする
		fx.isArmorHit = (enemy && enemy->IsKnockbackImmune());
		// 弾かれたヒットでは火花もリングも出さない。
		// 「攻撃が通っていない」ことは敵側の紫の火花（BossArmorHitSpark）が伝える
		fx.vfxName = fx.isArmorHit ? "" : "HitImpact";
		HitEffectSystem::GetInstance().Play(fx);

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

