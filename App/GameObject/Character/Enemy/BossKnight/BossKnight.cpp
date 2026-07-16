#include "BossKnight.h"
#include "GameObject/Character/Player/Player.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <World3D/Collider/AABBCollider.h>
#include <World3D/Collider/OBBCollider.h>
#include <World3D/Collider/CollisionManager.h>
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/State/EnemyStateAir.h"
#include "GameObject/Character/Enemy/State/EnemyStateKnockBack.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"

#include "State/BossStateCombatIdle.h"
#include "State/BossStateApproach.h"
#include "State/BossStateSlash.h"
#include "State/BossStateHeavySword.h"
#include "State/BossStateRush.h"
#include "State/BossStateKnockBack.h"

BossKnight::BossKnight(std::string objectName) : Enemy(objectName) {
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));
	AddRenderer(RendererManager::GetInstance().FindRender(name_));
	GetRenderer(name_)->GetWorldTransform()->GetScale() = { 1.5f, 1.5f, 1.5f };

	hp_ = kMaxHp;
	maxHp_ = kMaxHp;
}

void BossKnight::Initialize() {
	// ── 自コライダーの調整 ──
	auto* col = static_cast<OBBCollider*>(GetCollider(name_));
	col->GetColliderData().halfExtents *= 0.65f;

	// ── 武器の生成 ──
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_ + "Weapon", "Sword"));
	CollisionManager::GetInstance().AddCollider(std::make_unique<AABBCollider>(name_ + "Weapon"));
	CollisionManager::GetInstance().FindCollider(name_ + "Weapon")->category_ = CollisionCategory::EnemyWeapon;

	auto weapon = std::make_unique<BossWeapon>(name_ + "Weapon");
	weapon->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	weapon->AddCollider(CollisionManager::GetInstance().FindCollider(name_ + "Weapon"));
	weapon->Initialize();
	weapon->GetWorldTransform()->SetParent(GetWorldTransform());
	weapon_ = weapon.get();
	Object3dManager::GetInstance().AddObject(std::move(weapon));

	// ── コンポーネント生成 ──
	sensor_ = std::make_unique<EnemySensorComponent>();
	sensor_->SetDetectionRange(25.0f); // 広い感知範囲
	movement_ = std::make_unique<EnemyMovementComponent>();
	meleeAttack_ = std::make_unique<EnemyMeleeAttackComponent>(weapon_);

	// ── ステート登録 ──
	states_[EnemyStateName::Air] = std::make_unique<EnemyStateAir>();
	// 共有の KnockBack は軽いのけぞり(HitStun)用。着地後は CombatIdle へ戻る。
	states_[EnemyStateName::KnockBack] = std::make_unique<EnemyStateKnockBack>();
	// ボス専用の吹き飛び。着地後すぐ Rush(ダッシュ攻撃)へ移行する。
	states_[BossStateName::KnockBack] = std::make_unique<BossStateKnockBack>();

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
	ParticleManager::GetInstance().CreateEmitter(name_ + "HitEffect", "EnemyDamageEmitter");
	auto& emitters = ParticleManager::GetInstance().GetEmitters();
	hitEmitter_ = emitters.at(name_ + "HitEffect").get();
	hitEmitter_->SetParent(GetWorldTransform());
	hitEmitter_->AddParticle("EnemyDamageEffect");
	hitEmitter_->AddParticle("PlayerSlashEffect");
	hitEmitter_->SetActiveFlag(false);

	// ── パーティクル: チャージエフェクト（予備動作中に収束するリング）──
	ParticleManager::GetInstance().CreateEmitter(name_ + "ChargeEffect");
	chargeEmitter_ = ParticleManager::GetInstance().GetEmitters().at(name_ + "ChargeEffect").get();
	chargeEmitter_->SetParent(GetWorldTransform());
	chargeEmitter_->AddParticle("EnemyChargeRing");

	Enemy::Initialize();

	// 出現・死亡演出のディゾルブ対象に武器も含める
	if (appearanceFx_) {
		appearanceFx_->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	}

	// 初期ステートを CombatIdle に設定（Enemy::Initialize は "Air" にセットする）
	ChangeState(BossStateName::CombatIdle);
}

void BossKnight::Update(float deltaTime) {
	// 死亡演出終了後は武器が後始末済みのため、本体の後始末だけ行う
	if (!IsAlive()) {
		Enemy::Update(deltaTime);
		return;
	}

	// KnockBack は velocity_.y を直接操作するため重力の二重適用を避ける（のけぞり・吹き飛び両方）
	bool isKnockBack = (currentState_ == states_.at(EnemyStateName::KnockBack).get())
		|| (currentState_ == states_.at(BossStateName::KnockBack).get());
	if (!isKnockBack) {
		SetAcceleration({ 0.0f, GetOnGround() ? 0.0f : -9.8f, 0.0f });
	}

	// 武器は常に表示し、攻撃中以外はデフォルトポーズに戻す
	weapon_->SetIsDraw(true);

	if (meleeAttack_->IsFinished()) {
		weapon_->GetWorldTransform()->GetTranslation() = { 0.0f, 0.1f, 0.5f };
		weapon_->GetWorldTransform()->GetRotation() = EulerDegree({ 0.0f, 90.0f, 150.0f });
	}

	// 攻撃フェーズのみ武器コライダーを有効化（出現/死亡演出中は無効）
	bool isAttackPhase = !meleeAttack_->IsFinished() && !meleeAttack_->IsWindingUp()
		&& !IsAppearanceEffectPlaying();
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

void BossKnight::OnDeathEffectFinished() {
	// 武器を後始末する（本体は Enemy::Update の !isAlive_ 側で後始末される）
	if (weapon_) {
		weapon_->isAlive = false;
		weapon_->ResetObject();
		weapon_ = nullptr;
	}
}

void BossKnight::OnCollisionEnter(BaseCollider* other) {
	Enemy::OnCollisionEnter(other);

	if (other->category_ != CollisionCategory::PlayerWeapon) return;
	if (!player_ || !player_->IsAttack()) return;
	// 出現・死亡演出中は被弾処理をしない
	if (IsAppearanceEffectPlaying()) return;

	const AttackData atk = player_->GetAttackData(); // 値返しなのでローカルにコピー
	const float damage = atk.damage;

	// 常にヒットストップとエフェクトは再生する
	hitStop_->Start(atk.hitStopTime, atk.hitStopIntensity * 3.0f);
	hitEmitter_->Emit();

	// ── スーパーアーマー ───────────────────────────────────────────────
	// 吹き飛び中(BossKnockBack)とダッシュ攻撃中(Rush)は、ダメージは通すが
	// ひるみ・再ノックバックはさせない。
	// → 「ノックバックが始まったら飛び切る」「Rushが終わるまでひるまない」を保証する。
	const bool inFlyKnockback = (currentState_ == states_.at(BossStateName::KnockBack).get());
	const bool inRush         = (currentState_ == states_.at(BossStateName::Rush).get());
	if (inFlyKnockback || inRush) {
		hp_ -= damage;
		if (hp_ <= 0.0f) OnDeath();
		return;
	}

	// ── 通常の被弾（CombatIdle・通常攻撃中・のけぞり中）────────────────
	hp_ -= damage;
	hitAccumulation_ += damage;

	DamageInfo info;
	info.damage = damage;
	info.hitPosition = GetWorldTransform()->GetTranslation();
	info.attackerPosition = player_->GetWorldTransform()->GetTranslation();
	info.direction = Normalize(info.hitPosition - info.attackerPosition);

	if (hp_ <= 0.0f) {
		OnDeath();
		// 死亡演出中はステート更新が止まるため、吹き飛びの初速を直接与える
		Vector3 deathVelocity = info.direction * atk.impulseForce;
		deathVelocity.y += atk.impulseForce * atk.upwardRatio;
		SetVelocity(deathVelocity);
		SetOnGround(false);
		return;
	}

	// ノックバック優先: ノックバック/打ち上げ系の攻撃、または蓄積ダメージが閾値を超えたら吹き飛ばす。
	// のけぞり(共有KnockBack)中でもこの分岐に入るため、フィニッシュのノックバックがのけぞりに潰されない。
	const bool wantKnockback =
		(atk.type != ReactionType::HitStun) || (hitAccumulation_ >= kKnockbackThreshold);

	if (wantKnockback) {
		hitAccumulation_ = 0.0f;

		// HitStun系の攻撃でも閾値ブレイク時は確実に飛ばすため Knockback に昇格する
		info.type = (atk.type == ReactionType::HitStun) ? ReactionType::Knockback : atk.type;
		info.impulseForce = atk.impulseForce;
		info.upwardRatio = atk.upwardRatio;
		info.torqueForce = atk.torqueForce;
		info.stunTime = atk.stunTime;

		SetPendingDamageInfo(info);
		ChangeState(BossStateName::KnockBack); // 吹き飛び → 着地後すぐ Rush(ダッシュ攻撃)
	} else {
		// ── 閾値未満: 短いのけぞり（HitStun）のみ ─────────────────
		info.type = ReactionType::HitStun;
		info.impulseForce = atk.impulseForce * 0.2f;
		info.stunTime = 0.15f; // 短い硬直で次の攻撃を入れやすくする

		SetPendingDamageInfo(info);
		ChangeState(EnemyStateName::KnockBack); // 共有のけぞり → CombatIdle
	}
}

void BossKnight::OnCollisionStay(BaseCollider* other) { Enemy::OnCollisionStay(other); }
void BossKnight::OnCollisionExit(BaseCollider* other) { Enemy::OnCollisionExit(other); }
