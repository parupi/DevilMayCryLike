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
}

void GruntMelee::Initialize() {
	auto* col = static_cast<OBBCollider*>(GetCollider(name_));
	col->GetColliderData().halfExtents *= 0.5f;

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

	// 初期ステートを Patrol へ上書き（Enemy::Initialize() は "Air" にセットする）
	ChangeState(GruntMeleeStateName::Patrol);
}

void GruntMelee::Update(float deltaTime) {
	// 武器は常に表示し、攻撃中以外はデフォルトポーズに戻す
	weapon_->SetIsDraw(true);

	if (meleeAttack_->IsFinished()) {
		weapon_->GetWorldTransform()->GetTranslation() = { 0.0f, 0.1f, -0.5f };
		weapon_->GetWorldTransform()->GetRotation()    = EulerDegree({ 0.0f, 90.0f, 150.0f });
	}

	// 攻撃フェーズのみ武器コライダーを有効化（予備動作・非攻撃時は無効）
	bool isAttackPhase = !meleeAttack_->IsFinished() && !meleeAttack_->IsWindingUp();
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

	if (currentState_ == states_[EnemyStateName::KnockBack].get()) {
		hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
		emitter_->Emit();
		return;
	}

	hp_ -= player_->GetAttackData().damage;
	if (hp_ <= 0.0f) {
		OnDeath();
	}

	DamageInfo info;
	info.damage = player_->GetAttackData().damage;
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

	hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
	emitter_->Emit();
}

void GruntMelee::OnCollisionStay(BaseCollider* other) {
	Enemy::OnCollisionStay(other);
}

void GruntMelee::OnCollisionExit(BaseCollider* other) {
	Enemy::OnCollisionExit(other);
}
