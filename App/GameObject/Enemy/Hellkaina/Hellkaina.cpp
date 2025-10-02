#include "Hellkaina.h"
#include "GameObject/Player/Player.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "State/EnemyStateIdle.h"
#include "State/EnemyStateAir.h"
#include "State/EnemyStateMove.h"
#include "State/HellkainaStateKnockBack.h"

Hellkaina::Hellkaina(std::string objectName) : Enemy(objectName)
{
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));

	//CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("EnemyCollider"));

	AddRenderer(RendererManager::GetInstance()->FindRender(name_));
	//AddCollider(CollisionManager::GetInstance()->FindCollider("EnemyCollider"));

	states_["Idle"] = std::make_unique<EnemyStateIdle>();
	states_["Move"] = std::make_unique<EnemyStateMove>();
	states_["Air"] = std::make_unique<EnemyStateAir>();
	states_["KnockBack"] = std::make_unique<HellkainaStateKnockBack>();
	currentState_ = states_["Move"].get();

	hp_ = 10.0f;
}

void Hellkaina::Initialize()
{
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMin *= 0.5f;

	//GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };

	hitStop_ = std::make_unique<HitStop>();
}

void Hellkaina::Update()
{


	Enemy::Update();
}

void Hellkaina::OnCollisionEnter(BaseCollider* other)
{
	Enemy::OnCollisionEnter(other);

	if (other->category_ == CollisionCategory::PlayerWeapon) {
		if (currentState_ != states_["KnockBack"].get()) {
			ChangeState("KnockBack");
			hp_ -= player_->GetAttackData().damage;
			velocity_ = player_->GetAttackData().knockBackSpeed;
		}
		hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
		slashEmitter_->Emit();
	}
}

void Hellkaina::OnCollisionStay(BaseCollider* other)
{

	Enemy::OnCollisionStay(other);

	if (other->category_ == CollisionCategory::PlayerWeapon) {
		if (currentState_ != states_["KnockBack"].get()) {
			ChangeState("KnockBack");
			hp_ -= player_->GetAttackData().damage;
			// 速度を設定
			velocity_ = RotateVector(player_->GetAttackData().knockBackSpeed, player_->GetWorldTransform()->GetRotation());
		}
	}
}

void Hellkaina::OnCollisionExit(BaseCollider* other)
{
	Enemy::OnCollisionExit(other);
}
