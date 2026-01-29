#include "Hellkaina.h"
#include "GameObject/Character/Player/Player.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "State/EnemyStateIdle.h"
#include "State/EnemyStateAir.h"
#include "State/EnemyStateMove.h"
#include "State/HellkainaStateKnockBack.h"
#include <scene/Transition/TransitionManager.h>
#include <3d/Collider/CollisionManager.h>
#include "State/HellkainaStateAttackA.h"
#include "State/HellkainaStateAttackB.h"
#include "State/HellkainaStateSideMove.h"

Hellkaina::Hellkaina(std::string objectName) : Enemy(objectName)
{
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));

	AddRenderer(RendererManager::GetInstance()->FindRender(name_));

	GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };

	states_["Idle"] = std::make_unique<EnemyStateIdle>();
	states_["Move"] = std::make_unique<EnemyStateMove>();
	states_["SideMove"] = std::make_unique<HellkainaStateSideMove>();
	states_["Air"] = std::make_unique<EnemyStateAir>();
	states_["KnockBack"] = std::make_unique<HellkainaStateKnockBack>();
	states_["AttackA"] = std::make_unique<HellkainaStateAttackA>();
	states_["AttackB"] = std::make_unique<HellkainaStateAttackB>();
	currentState_ = states_["Air"].get();

	hp_ = 10.0f;

	slashEmitter_ = std::make_unique<ParticleEmitter>();
	slashEmitter_->Initialize(name_ + "slash");
	slashEmitter_->SetParticle("test");
	slashEmitter_->SetParent(GetWorldTransform());

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize(name_ + "HitSmoke");
	smokeEmitter_->SetParticle("hitSmoke");
	smokeEmitter_->SetParent(GetWorldTransform());
}

void Hellkaina::Initialize()
{
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMin *= 0.5f;

	// 武器のレンダラー生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_ + "HellkainaWeapon", "Sword"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>(name_ + "HellkainaWeapon"));
	// 武器を生成
	std::unique_ptr<HellkainaWeapon> weapon = std::make_unique<HellkainaWeapon>(name_ + "HellkainaWeapon");

	weapon->AddRenderer(RendererManager::GetInstance()->FindRender(name_ + "HellkainaWeapon"));

	weapon->AddCollider(CollisionManager::GetInstance()->FindCollider(name_ + "HellkainaWeapon"));

	weapon->Initialize();

	weapon->GetWorldTransform()->SetParent(GetWorldTransform());

	weapon_ = weapon.get();

	Object3dManager::GetInstance()->AddObject(std::move(weapon));

	hitStop_ = std::make_unique<HitStop>();

	Enemy::Initialize();
}

void Hellkaina::Update(float deltaTime)
{

	//weapon_->Update();
	weapon_->SetIsDraw(isAttack_);
	Enemy::Update(deltaTime);
}

#ifdef _DEBUG
void Hellkaina::DebugGui()
{
	ImGui::Begin(name_.c_str());
	Object3d::DebugGui();
	ImGui::End();
	//std::string weaponName = name_ + "weapon";
	//ImGui::Begin(weaponName.c_str());
	//weapon_->DebugGui();
	//ImGui::End();
	
}
#endif // DEBUG

void Hellkaina::OnCollisionEnter(BaseCollider* other)
{
	if (!player_) return;

	Enemy::OnCollisionEnter(other);

	if (other->category_ == CollisionCategory::PlayerWeapon) {
		if (currentState_ != states_["KnockBack"].get()) {
			ChangeState("KnockBack");
			hp_ -= player_->GetAttackData().damage;
			// プレイヤーの位置に応じて速度を設定
			Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
			Vector3 direction = Normalize(GetWorldTransform()->GetTranslation() - playerPos);
			Vector3 newVelocity = direction * player_->GetAttackData().knockBackSpeed.z;
			velocity_ = newVelocity;
			// y軸はプレイヤーの位置にかかわらず一定
			velocity_.y = player_->GetAttackData().knockBackSpeed.y;
		}
		hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
		slashEmitter_->Emit();
	}
}

void Hellkaina::OnCollisionStay(BaseCollider* other)
{
	//RotateVector(player_->GetAttackData().knockBackSpeed, player_->GetWorldTransform()->GetRotation());
	Enemy::OnCollisionStay(other);

	//if (other->category_ == CollisionCategory::PlayerWeapon) {
	//	if (currentState_ != states_["KnockBack"].get()) {
	//		ChangeState("KnockBack");
	//		hp_ -= player_->GetAttackData().damage;
	//		// 速度を設定
	//		Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
	//		Vector3 direction = Normalize(GetWorldTransform()->GetTranslation() - playerPos);
	//		Vector3 newVelocity = direction * player_->GetAttackData().knockBackSpeed;
	//		velocity_ = newVelocity;
	//	}
	//}
}

void Hellkaina::OnCollisionExit(BaseCollider* other)
{
	Enemy::OnCollisionExit(other);
}
