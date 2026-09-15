#include "PlayerWeapon.h"
#include "World3D/Collider/OBBCollider.h"
#include "Player.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "Scene/Transition/TransitionManager.h"
#include "Math/MathUtils.h"
#include "GameObject/Effect/HitEffectSystem.h"
#include "Audio/SoundManager.h"
#include <algorithm>
#include <cmath>

namespace {
	// 攻撃名からチュートリアルの種類を判定する（該当しない攻撃はCountを返す）
	TutorialState ResolveTutorialState(const std::string& attackName) {
		if (attackName.rfind("AttackComboA", 0) == 0) return TutorialState::AttackA;
		if (attackName.rfind("AttackComboB", 0) == 0) return TutorialState::AttackB;
		if (attackName == "AttackHighTime") return TutorialState::RoundUpAttack;
		return TutorialState::Count;
	}

	Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback) {
		const float length = Length(v);
		return (length > 0.0001f) ? v * (1.0f / length) : fallback;
	}

	// 刃（base→tip の線分）の上で、敵の体の中心に一番近い点。
	// そこから少し体の中へ寄せた位置を「斬った場所」とする。
	// 以前は敵の原点（体の中心）から出していたので、どこを斬っても同じ場所から火花が出ていた
	Vector3 CalcBladeHitPosition(const Vector3& bladeBase, const Vector3& bladeTip, const Vector3& bodyCenter) {
		const Vector3 blade = bladeTip - bladeBase;
		const float lengthSq = Dot(blade, blade);
		float t = (lengthSq > 0.0001f) ? Dot(bodyCenter - bladeBase, blade) / lengthSq : 1.0f;
		t = std::clamp(t, 0.0f, 1.0f);
		const Vector3 onBlade = bladeBase + blade * t;
		return onBlade + (bodyCenter - onBlade) * 0.35f;
	}

	// 材質ごとの追加VFX（Resource/VFX/HitMat*.vfx.json）
	const char* MaterialVfxName(Enemy::HitMaterial material) {
		switch (material) {
		case Enemy::HitMaterial::Bone:  return "HitMatBone";
		case Enemy::HitMaterial::Armor: return "HitMatArmor";
		case Enemy::HitMaterial::Wood:  return "HitMatWood";
		case Enemy::HitMaterial::Flesh:
		default:                        return "HitMatFlesh";
		}
	}
}

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName) {}

void PlayerWeapon::Initialize() {
	Object3d::Initialize();

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {0.5f, 1.0f, 0.5f};

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<OBBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().halfExtents = baseHalfExtents_;

	defaultPosition_ = {0.0f, 0.1f, -0.5f};
	defaultRotation_ = {0.0f, 90.0f, 150.0f};

	GetWorldTransform()->GetTranslation() = defaultPosition_;
	GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
}

void PlayerWeapon::Update(float deltaTime) {
	if (!player_) return;

	auto* collider = static_cast<OBBCollider*>(GetCollider("WeaponCollider"));
	// 攻撃中だけ判定を出す。
	// 多段ヒットの区切りでは1回だけ切る。CollisionManager は「前回も触れていたか」で Enter と Stay を
	// 分けるので、一度離れたことにしないと、触れたままの敵へ次の段が入らない
	collider->GetColliderData().isActive = isAttack_ && !rehitRequested_;
	rehitRequested_ = false;
	// 攻撃ごとの当たり判定の大きさ（回転攻撃などで広げる）
	const float hitboxScale = player_->IsAttack() ? player_->GetAttackData().hitboxScale : 1.0f;
	collider->GetColliderData().halfExtents = baseHalfExtents_ * hitboxScale;

	// 剣の軌跡は PlayerAttackEffect が刃先・根本の位置から描く
	Object3d::Update(deltaTime);
}

void PlayerWeapon::Draw() {
	Object3d::Draw();
}

Vector3 PlayerWeapon::GetBladeTipWorld() {
	return Transform(bladeTipOffset_, GetWorldTransform()->GetMatWorld());
}

Vector3 PlayerWeapon::GetBladeBaseWorld() {
	return Transform(bladeBaseOffset_, GetWorldTransform()->GetMatWorld());
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
		hitCtx.isStrong = (attack.knockback.type == ReactionType::Launch || attack.knockback.type == ReactionType::Knockback);
		auto* enemy = dynamic_cast<Enemy*>(other->owner_);
		if (enemy) {
			hitCtx.enemyMultiplier = enemy->GetStyleMultiplier();
		}
		scoreManager_->OnAttackHit(hitCtx);

		// ── ヒット演出（VFX・カメラシェイク・ポストエフェクト・ヒットストップ・ライト）──
		// 中身は HitEffectSystem がまとめて面倒を見る。ここでは「どこで・どちらへ・どの種類で」を決める
		const Vector3 attackerPosition = player_->GetWorldTransform()->GetTranslation();
		const Vector3 bladeTip = GetBladeTipWorld();
		const Vector3 bladeBase = GetBladeBaseWorld();
		const PlayerAttackEffect* attackEffect = player_->GetAttackEffect();

		HitEffectRequest fx;
		// 斬った場所: 刃の上で敵の体に一番近い点
		if (enemy) {
			fx.position = CalcBladeHitPosition(bladeBase, bladeTip, enemy->GetBodyCenter());
		} else if (other->owner_) {
			fx.position = other->owner_->GetWorldTransform()->GetTranslation();
		} else {
			fx.position = bladeTip;
		}

		// 火花は剣を振っている向きへ流す（斬った向きが見えるように）。
		// 刃がほとんど動いていないとき（突き・溜めの出始め）は、攻撃者から離れる向き
		const Vector3 away = SafeNormalize(fx.position - attackerPosition, Vector3{ 0.0f, 1.0f, 0.0f });
		const Vector3 tipVelocity = attackEffect ? attackEffect->GetTipVelocity() : Vector3{};
		if (Length(tipVelocity) > 1.0f) {
			fx.direction = SafeNormalize(SafeNormalize(tipVelocity, away) * 0.75f + away * 0.25f, away);
		} else {
			fx.direction = away;
		}

		fx.strength = attack.hitStopStrength;
		fx.hitStopTime = attack.hitStopTime;
		fx.hitStopIntensity = attack.hitStopIntensity;
		// スーパーアーマーで弾かれた場合は控えめな演出にする
		fx.isArmorHit = (enemy && enemy->IsKnockbackImmune());

		// 弾かれたヒットでは火花もリングも出さない。
		// 「攻撃が通っていない」ことは敵側の紫の火花（BossArmorHitSpark）が伝える
		if (!fx.isArmorHit) {
			const AttackVfxStyle style = attackEffect ? attackEffect->GetCurrentStyle() : AttackVfxStyle::Slash;
			const bool isHeavy = (style == AttackVfxStyle::Heavy || style == AttackVfxStyle::Slam || style == AttackVfxStyle::Thrust);
			// 通常斬りは白＋青の十字の光、強攻撃は白＋金の大きな光
			fx.vfxName = isHeavy ? "PlayerHitHeavy" : "PlayerHitSlash";
			fx.materialVfxName = enemy ? MaterialVfxName(enemy->GetHitMaterial()) : "";
		}
		// 大きな敵ほど火花を少し大きくする（ボスは配置スケール2で約1.4倍）
		if (enemy) {
			const float enemyScale = enemy->GetWorldTransform()->GetWorldScale().y;
			fx.sizeScale = std::clamp(std::sqrt((std::max)(enemyScale, 0.0f)), 1.0f, 1.6f);
		}
		HitEffectSystem::GetInstance().Play(fx);

		// 手応えの音。弾かれたヒットは通っていないので控えめに鳴らす
		SoundManager::GetInstance().PlaySE("SwordHit", fx.isArmorHit ? 0.5f : 0.9f);

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

