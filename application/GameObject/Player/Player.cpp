#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "State/PlayerStateAttack1.h"
#include <Renderer/RendererManager.h>
#include <Renderer/PrimitiveRenderer.h>
#include <Collider/AABBCollider.h>
#include <Collider/CollisionManager.h>

Player::Player() : Object3d()
{
	states_["Idle"] = std::make_unique<PlayerStateIdle>();
	states_["Move"] = std::make_unique<PlayerStateMove>();
	states_["Attack1"] = std::make_unique<PlayerStateAttack1>();
	currentState_ = states_["Idle"].get();
}

void Player::Initialize()
{
	Object3d::Initialize();

	GetRenderer("PlayerLeftArm")->GetWorldTransform()->GetTranslation() = { -0.4f, 0.8f, 0.0f };
	GetRenderer("PlayerRightArm")->GetWorldTransform()->GetTranslation() = { 0.4f, 0.8f, 0.0f };

	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Cylinder1", PrimitiveRenderer::PrimitiveType::Cylinder, "MagicEffect.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Cylinder2", PrimitiveRenderer::PrimitiveType::Cylinder, "MagicEffect.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Cylinder3", PrimitiveRenderer::PrimitiveType::Cylinder, "gradationLine_brightened.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Cylinder4", PrimitiveRenderer::PrimitiveType::Cylinder, "gradationLine_brightened.png"));

	attackEfect_ = std::make_unique<PlayerAttackEffect>();
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder1"));
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder2"));
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder3"));
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder4"));
	attackEfect_->Initialize();

	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "weapon"));

	std::unique_ptr<AABBCollider> playerCollider = std::make_unique<AABBCollider>("WeaponCollider");
	CollisionManager::GetInstance()->AddCollider(std::move(playerCollider));

	weapon_ = std::make_unique<PlayerWeapon>();

	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));

	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));

	weapon_->Initialize();

}

void Player::Update()
{
	if (currentState_) {
		currentState_->Update(*this);
	}

	GetWorldTransform()->GetTranslation() += velocity_;


	attackEfect_->Update();

	attackEfect_->UpdateAttackCylinderEffect(GetWorldTransform()->GetTranslation());
	attackEfect_->UpdateTargetMarkerEffect({ 0.0f, 0.0f, 5.0f });


	weapon_->Update();
	Object3d::Update();


}

void Player::Draw()
{
	weapon_->Draw();
	Object3d::Draw();
}

void Player::DrawEffect()
{
	attackEfect_->Draw();
}

#ifdef _DEBUG
void Player::DebugGui()
{
	ImGui::Begin("Effect");
	attackEfect_->DebugGui();
	ImGui::End();

	ImGui::Begin("Object");
	Object3d::DebugGui();
	ImGui::End();

	ImGui::Begin("Weapon");
	weapon_->DebugGui();
	ImGui::End();


}

#endif // _DEBUG

void Player::ChangeState(const std::string& stateName)
{
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}

void Player::OnCollisionEnter(BaseCollider* other)
{
}

void Player::OnCollisionStay(BaseCollider* other)
{
}

void Player::OnCollisionExit(BaseCollider* other)
{
}
