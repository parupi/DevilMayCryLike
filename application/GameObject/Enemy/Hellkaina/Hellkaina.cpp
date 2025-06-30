#include "Hellkaina.h"

#include <Renderer/RendererManager.h>
#include <Collider/AABBCollider.h>
#include <Renderer/ModelRenderer.h>
#include "State/EnemyStateIdle.h"
#include "State/EnemyStateAir.h"
#include "State/EnemyStateMove.h"

Hellkaina::Hellkaina(std::string objectName) : Enemy(objectName)
{
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name, "PlayerBody"));

	//CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("EnemyCollider"));

	AddRenderer(RendererManager::GetInstance()->FindRender(name));
	//AddCollider(CollisionManager::GetInstance()->FindCollider("EnemyCollider"));

	states_["Idle"] = std::make_unique<EnemyStateIdle>();
	states_["Move"] = std::make_unique<EnemyStateMove>();
	states_["Air"] = std::make_unique<EnemyStateAir>();
	currentState_ = states_["Move"].get();
}

void Hellkaina::Initialize()
{
	static_cast<AABBCollider*>(GetCollider(name))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name))->GetColliderData().offsetMin *= 0.5f;

	GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };
}

void Hellkaina::Update()
{
	Enemy::Update();
}
