#include "Hellkaina.h"

#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "State/EnemyStateIdle.h"
#include "State/EnemyStateAir.h"
#include "State/EnemyStateMove.h"

Hellkaina::Hellkaina(std::string objectName) : Enemy(objectName)
{
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));

	//CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("EnemyCollider"));

	AddRenderer(RendererManager::GetInstance()->FindRender(name_));
	//AddCollider(CollisionManager::GetInstance()->FindCollider("EnemyCollider"));

	states_["Idle"] = std::make_unique<EnemyStateIdle>();
	states_["Move"] = std::make_unique<EnemyStateMove>();
	states_["Air"] = std::make_unique<EnemyStateAir>();
	currentState_ = states_["Move"].get();
}

void Hellkaina::Initialize()
{
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMin *= 0.5f;

	GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };
}

void Hellkaina::Update()
{
	Enemy::Update();
}
