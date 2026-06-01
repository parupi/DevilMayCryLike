#include "BossKnight.h"
#include "GameObject/Character/Player/Player.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Collider/CollisionManager.h>
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/State/EnemyStateAir.h"
#include "GameObject/Character/Enemy/State/EnemyStateKnockBack.h"
#include "base/Particle/ParticleManager.h"

#include "State/BossStateCombatIdle.h"
#include "State/BossStateApproach.h"
#include "State/BossStateSlash.h"
#include "State/BossStateHeavySword.h"
#include "State/BossStateRush.h"

BossKnight::BossKnight(std::string objectName) : Enemy(objectName) {
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));
	AddRenderer(RendererManager::GetInstance()->FindRender(name_));
	GetRenderer(name_)->GetWorldTransform()->GetScale() = { 1.5f, 1.5f, 1.5f };

	hp_ = kMaxHp;
}

void BossKnight::Initialize() {
	// ── 自コライダーの調整 ──
	auto* col = static_cast<AABBCollider*>(GetCollider(name_));
	col->GetColliderData().offsetMax *= 0.65f;
	col->GetColliderData().offsetMin *= 0.65f;

	// ── 武器の生成 ──
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_ + "Weapon", "Sword"));
	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>(name_ + "Weapon"));
	CollisionManager::GetInstance()->FindCollider(name_ + "Weapon")->category_ = CollisionCategory::EnemyWeapon;

	auto weapon = std::make_unique<BossWeapon>(name_ + "Weapon");
	weapon->AddRenderer(RendererManager::GetInstance()->FindRender(name_ + "Weapon"));
	weapon->AddCollider(CollisionManager::GetInstance()->FindCollider(name_ + "Weapon"));
	weapon->Initialize();
	weapon->GetWorldTransform()->SetParent(GetWorldTransform());
	weapon_ = weapon.get();
	Object3dManager::GetInstance()->AddObject(std::move(weapon));

	// ── コンポーネント生成 ──
	sensor_ = std::make_unique<EnemySensorComponent>();
	sensor_->SetDetectionRange(25.0f); // 広い感知範囲
	movement_ = std::make_unique<EnemyMovementComponent>();
	meleeAttack_ = std::make_unique<EnemyMeleeAttackComponent>(weapon_);

	// ── ステート登録 ──
	states_[EnemyStateName::Air] = std::make_unique<EnemyStateAir>();
	states_[EnemyStateName::KnockBack] = std::make_unique<EnemyStateKnockBack>();

	// EnemyStateKnockBack が着地後に "Idle" へ、EnemyStateAir が着地後に "Move" へ遷移する。
	// どちらも BossStateCombatIdle にマップして動作を引き継ぐ。
	states_[EnemyStateName::Idle] = std::make_unique<BossStateCombatIdle>(sensor_.get(), movement_.get(), kMaxHp);
	states_[EnemyStateName::Move] = std::make_unique<BossStateCombatIdle>(sensor_.get(), movement_.get(), kMaxHp);

	states_[BossStateName::CombatIdle] = std::make_unique<BossStateCombatIdle>(sensor_.get(), movement_.get(), kMaxHp);
	states_[BossStateName::Approach] = std::make_unique<BossStateApproach>(movement_.get());
	states_[BossStateName::Slash] = std::make_unique<BossStateSlash>(meleeAttack_.get());
	states_[BossStateName::HeavySword] = std::make_unique<BossStateHeavySword>(meleeAttack_.get());
	states_[BossStateName::Rush] = std::make_unique<BossStateRush>(meleeAttack_.get());

	// ── パーティクル: ヒットエフェクト ──
	ParticleManager::GetInstance()->CreateEmitter(name_ + "HitEffect", "EnemyDamageEmitter");
	auto& emitters = ParticleManager::GetInstance()->GetEmitters();
	hitEmitter_ = emitters.at(name_ + "HitEffect").get();
	hitEmitter_->SetParent(GetWorldTransform());
	hitEmitter_->AddParticle("EnemyDamageEffect");
	hitEmitter_->AddParticle("PlayerSlashEffect");
	hitEmitter_->SetActiveFlag(false);

	// ── パーティクル: チャージエフェクト（予備動作中に収束するリング）──
	ParticleManager::GetInstance()->CreateEmitter(name_ + "ChargeEffect");
	chargeEmitter_ = ParticleManager::GetInstance()->GetEmitters().at(name_ + "ChargeEffect").get();
	chargeEmitter_->SetParent(GetWorldTransform());
	chargeEmitter_->AddParticle("EnemyChargeRing");

	Enemy::Initialize();

	// 初期ステートを CombatIdle に設定（Enemy::Initialize は "Air" にセットする）
	ChangeState(BossStateName::CombatIdle);
}

void BossKnight::Update(float deltaTime) {
	// 武器は常に表示し、攻撃中以外はデフォルトポーズに戻す
	weapon_->SetIsDraw(true);

	if (meleeAttack_->IsFinished()) {
		weapon_->GetWorldTransform()->GetTranslation() = { 0.0f, 0.1f, 0.5f };
		weapon_->GetWorldTransform()->GetRotation() = EulerDegree({ 0.0f, 90.0f, 150.0f });
	}

	// 攻撃フェーズのみ武器コライダーを有効化
	bool isAttackPhase = !meleeAttack_->IsFinished() && !meleeAttack_->IsWindingUp();
	auto* weaponCol = static_cast<AABBCollider*>(weapon_->GetCollider(name_ + "Weapon"));
	if (weaponCol) {
		weaponCol->GetColliderData().isActive = isAttackPhase;
	}

	// 予備動作中にチャージリングを発射（GruntMeleeより速め）
	if (meleeAttack_->IsWindingUp()) {
		chargeEmitTimer_ += deltaTime;
		if (chargeEmitTimer_ >= kChargeEmitInterval) {
			chargeEmitter_->Emit();
			chargeEmitTimer_ = 0.0f;
		}
	}
	else {
		chargeEmitTimer_ = 0.0f;
	}

	Enemy::Update(deltaTime);
}

#ifdef _DEBUG
void BossKnight::DebugGui() {
	ImGui::Begin(name_.c_str());
	ImGui::Text("HP: %.1f / %.1f", hp_, kMaxHp);
	ImGui::Text("Phase: %d", (hp_ > kMaxHp * 0.66f) ? 1 : (hp_ > kMaxHp * 0.33f) ? 2 : 3);
	Object3d::DebugGui();
	ImGui::End();
}
#endif

void BossKnight::OnCollisionEnter(BaseCollider* other) {
	Enemy::OnCollisionEnter(other);

	if (other->category_ != CollisionCategory::PlayerWeapon) return;
	if (!player_ || !player_->IsAttack()) return;

	// KnockBack 中に追加ヒットしても演出のみ（蓄積はリセット済み）
	if (currentState_ == states_[EnemyStateName::KnockBack].get()) {
		hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
		hitEmitter_->Emit();
		return;
	}

	const float damage = player_->GetAttackData().damage;
	hp_ -= damage;
	hitAccumulation_ += damage;

	// 常にヒットストップとエフェクトは再生する
	hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
	hitEmitter_->Emit();

	if (hp_ <= 0.0f) {
		OnDeath();
		return;
	}

	// ── 蓄積ダメージが閾値を超えたら本ノックバック ────────────────
	if (hitAccumulation_ >= kKnockbackThreshold) {
		hitAccumulation_ = 0.0f;

		DamageInfo info;
		info.damage = damage;
		info.hitPosition = GetWorldTransform()->GetTranslation();
		info.attackerPosition = player_->GetWorldTransform()->GetTranslation();
		info.direction = Normalize(info.hitPosition - info.attackerPosition);
		info.type = player_->GetAttackData().type;
		info.impulseForce = player_->GetAttackData().impulseForce;
		info.upwardRatio = player_->GetAttackData().upwardRatio;
		info.torqueForce = player_->GetAttackData().torqueForce;
		info.stunTime = player_->GetAttackData().stunTime;

		SetPendingDamageInfo(info);
		ChangeState(EnemyStateName::KnockBack);
	} else {
		// ── 閾値未満: 短いのけぞり（HitStun）のみ ─────────────────
		DamageInfo stunInfo;
		stunInfo.damage = damage;
		stunInfo.hitPosition = GetWorldTransform()->GetTranslation();
		stunInfo.attackerPosition = player_->GetWorldTransform()->GetTranslation();
		stunInfo.direction = Normalize(stunInfo.hitPosition - stunInfo.attackerPosition);
		stunInfo.type = ReactionType::HitStun;
		stunInfo.impulseForce = player_->GetAttackData().impulseForce * 0.2f;
		stunInfo.stunTime = 0.15f; // 短い硬直で次の攻撃を入れやすくする

		SetPendingDamageInfo(stunInfo);
		ChangeState(EnemyStateName::KnockBack);
	}
}

void BossKnight::OnCollisionStay(BaseCollider* other) { Enemy::OnCollisionStay(other); }
void BossKnight::OnCollisionExit(BaseCollider* other) { Enemy::OnCollisionExit(other); }
