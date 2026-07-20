#include "GruntMelee.h"
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

#include "State/GruntStatePatrol.h"
#include "State/GruntStateCombatIdle.h"
#include "State/GruntStateApproach.h"
#include "State/GruntStateSideMove.h"
#include "State/GruntStateRetreat.h"
#include "State/GruntStateAttackNormal.h"
#include "State/GruntStateRushAttack.h"

GruntMelee::GruntMelee(std::string objectName) : Enemy(objectName) {
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));
	AddRenderer(RendererManager::GetInstance().FindRender(name_));
	GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };

	hp_ = 8.0f;
	maxHp_ = hp_;
}

void GruntMelee::Initialize() {
	// レベルデータのコライダーサイズをそのまま使う
	// （旧: レベル側の×2補正を打ち消すための *0.5f があったが、×2補正の撤廃に伴い削除。実効サイズは変わらない）

	// 武器の生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_ + "Weapon", "Sword"));
	CollisionManager::GetInstance().AddCollider(std::make_unique<AABBCollider>(name_ + "Weapon"));
	// 武器コライダーカテゴリを設定（プレイヤーへのダメージ判定に使用）
	CollisionManager::GetInstance().FindCollider(name_ + "Weapon")->category_ = CollisionCategory::EnemyWeapon;

	auto weapon = std::make_unique<GruntMeleeWeapon>(name_ + "Weapon");
	weapon->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	weapon->AddCollider(CollisionManager::GetInstance().FindCollider(name_ + "Weapon"));
	weapon->Initialize();
	weapon->GetWorldTransform()->SetParent(GetWorldTransform());

	weapon_ = weapon.get();
	Object3dManager::GetInstance().AddObject(std::move(weapon));

	// コンポーネント生成
	sensor_ = std::make_unique<EnemySensorComponent>();
	movement_ = std::make_unique<EnemyMovementComponent>();
	meleeAttack_ = std::make_unique<EnemyMeleeAttackComponent>(weapon_);

	// ステート登録
	states_[EnemyStateName::Air] = std::make_unique<EnemyStateAir>();
	states_[EnemyStateName::KnockBack] = std::make_unique<EnemyStateKnockBack>();

	states_[GruntMeleeStateName::Patrol] = std::make_unique<GruntStatePatrol>(sensor_.get());
	// KnockBack/Air 終了後に EnemyStateName::Idle/Move へ戻るため CombatIdle を全キーで登録する
	states_[EnemyStateName::Idle] = std::make_unique<GruntStateCombatIdle>(sensor_.get());
	states_[EnemyStateName::Move] = std::make_unique<GruntStateCombatIdle>(sensor_.get());
	states_[GruntMeleeStateName::CombatIdle] = std::make_unique<GruntStateCombatIdle>(sensor_.get());
	states_[GruntMeleeStateName::Approach] = std::make_unique<GruntStateApproach>(movement_.get());
	states_[GruntMeleeStateName::SideMove] = std::make_unique<GruntStateSideMove>(movement_.get());
	states_[GruntMeleeStateName::Retreat] = std::make_unique<GruntStateRetreat>(movement_.get());
	states_[GruntMeleeStateName::AttackNormal] = std::make_unique<GruntStateAttackNormal>(meleeAttack_.get());
	states_[GruntMeleeStateName::RushAttack] = std::make_unique<GruntStateRushAttack>(meleeAttack_.get());

	currentState_ = states_[GruntMeleeStateName::Patrol].get();

	// パーティクル: ヒットエフェクト
	ParticleManager::GetInstance().CreateEmitter(name_ + "HitEffect", "EnemyDamageEmitter");
	auto& emitters = ParticleManager::GetInstance().GetEmitters();
	emitter_ = emitters.at(name_ + "HitEffect").get();
	emitter_->SetParent(GetWorldTransform());
	emitter_->AddParticle("EnemyDamageEffect");
	emitter_->AddParticle("PlayerSlashEffect");
	emitter_->SetActiveFlag(false);

	// パーティクル: チャージエフェクト（予備動作中に収束するリング）
	ParticleManager::GetInstance().CreateEmitter(name_ + "ChargeEffect");
	chargeEmitter_ = ParticleManager::GetInstance().GetEmitters().at(name_ + "ChargeEffect").get();
	chargeEmitter_->SetParent(GetWorldTransform());
	chargeEmitter_->AddParticle("EnemyChargeRing");

	Enemy::Initialize();

	// 出現・死亡演出のディゾルブ対象に武器も含める
	if (appearanceFx_) {
		appearanceFx_->AddRenderer(RendererManager::GetInstance().FindRender(name_ + "Weapon"));
	}

	// 初期ステートを Patrol へ上書き（Enemy::Initialize() は "Air" にセットする）
	ChangeState(GruntMeleeStateName::Patrol);
}

void GruntMelee::Update(float deltaTime) {
	// 死亡演出終了後は武器が後始末済みのため、本体の後始末だけ行う
	if (!IsAlive()) {
		Enemy::Update(deltaTime);
		return;
	}

	if (isActive_) {
		// 武器は常に表示し、攻撃中以外はデフォルトポーズに戻す
		weapon_->SetIsDraw(true);
	} else {
		// 未出現時は武器を隠し、非アクティブ時の共通処理（消灯など）だけ行う
		weapon_->SetIsDraw(false);
		Enemy::Update(deltaTime);
		return;
	}


	if (meleeAttack_->IsFinished()) {
		weapon_->GetWorldTransform()->GetTranslation() = { 0.0f, 0.1f, -0.5f };
		weapon_->GetWorldTransform()->GetRotation()    = EulerDegree({ 0.0f, 90.0f, 150.0f });
	}

	// 攻撃フェーズのみ武器コライダーを有効化（予備動作・非攻撃時・出現/死亡演出中は無効）
	bool isAttackPhase = !meleeAttack_->IsFinished() && !meleeAttack_->IsWindingUp()
		&& !IsAppearanceEffectPlaying();
	auto* weaponCol = static_cast<AABBCollider*>(weapon_->GetCollider(name_ + "Weapon"));
	if (weaponCol) {
		weaponCol->GetColliderData().isActive = isAttackPhase;
	}

	// 予備動作中にチャージリングを一定間隔で発射
	if (meleeAttack_->IsWindingUp()) {
		chargeEmitTimer_ += deltaTime;
		if (chargeEmitTimer_ >= kChargeEmitInterval) {
			chargeEmitter_->Emit();
			chargeEmitTimer_ = 0.0f;
		}
	} else {
		chargeEmitTimer_ = 0.0f;
	}

	Enemy::Update(deltaTime);
}

#ifdef _DEBUG
void GruntMelee::DebugGui() {
	ImGui::Begin(name_.c_str());
	Object3d::DebugGui();
	ImGui::End();
}
#endif

void GruntMelee::OnCollisionEnter(BaseCollider* other) {
	Enemy::OnCollisionEnter(other);

	if (other->category_ != CollisionCategory::PlayerWeapon) return;
	if (!player_ || !player_->IsAttack()) return;
	// 出現・死亡演出中は被弾処理をしない
	if (IsAppearanceEffectPlaying()) return;

	// 攻撃がヒットしたのでライトを強く光らせる
	FlashLight();

	if (currentState_ == states_[EnemyStateName::KnockBack].get()) {
		hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
		emitter_->Emit();
		return;
	}

	const AttackData atk = player_->GetAttackData(); // 値返しなのでローカルにコピー

	hp_ -= atk.damage;

	DamageInfo info;
	info.damage = atk.damage;
	info.hitPosition = GetWorldTransform()->GetTranslation();
	info.attackerPosition = player_->GetWorldTransform()->GetTranslation();
	info.direction = Normalize(info.hitPosition - info.attackerPosition);
	info.type = atk.type;
	info.impulseForce = atk.impulseForce;
	info.upwardRatio = atk.upwardRatio;
	info.torqueForce = atk.torqueForce;
	info.stunTime = atk.stunTime;

	if (hp_ <= 0.0f) {
		if (CanDie()) {
			OnDeath();
			// 死亡演出中はステート更新が止まるため、吹き飛びの初速を直接与える
			Vector3 deathVelocity = info.direction * atk.impulseForce;
			deathVelocity.y += atk.impulseForce * atk.upwardRatio;
			SetVelocity(deathVelocity);
			SetOnGround(false);
			emitter_->Emit();
			return;
		} else {
			// まだ死亡できない（チュートリアル中など）ので生存を維持する
			hp_ = 1.0f;
		}
	}

	SetPendingDamageInfo(info);
	ChangeState(EnemyStateName::KnockBack);

	hitStop_->Start(atk.hitStopTime, atk.hitStopIntensity * 3.0f);
	emitter_->Emit();
}

void GruntMelee::OnDeathEffectFinished() {
	// 武器を後始末する（本体は Enemy::Update の !isAlive_ 側で後始末される）
	if (weapon_) {
		weapon_->isAlive = false;
		weapon_->ResetObject();
		weapon_ = nullptr;
	}
}

void GruntMelee::OnCollisionStay(BaseCollider* other) {
	Enemy::OnCollisionStay(other);
}

void GruntMelee::OnCollisionExit(BaseCollider* other) {
	Enemy::OnCollisionExit(other);
}
