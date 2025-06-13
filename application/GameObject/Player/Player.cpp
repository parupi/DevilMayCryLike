#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "State/PlayerStateAttack1.h"
#include <Renderer/RendererManager.h>
#include <Renderer/PrimitiveRenderer.h>
#include <Collider/AABBCollider.h>
#include <Collider/CollisionManager.h>
#include <utility/DeltaTime.h>

Player::Player(std::string objectNama) : Object3d(objectNama)
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

	attackEfect_ = std::make_unique<PlayerAttackEffect>("Effect");
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder1"));
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder2"));
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder3"));
	attackEfect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder4"));
	attackEfect_->Initialize();

	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "weapon"));

	std::unique_ptr<AABBCollider> playerCollider = std::make_unique<AABBCollider>("WeaponCollider");
	CollisionManager::GetInstance()->AddCollider(std::move(playerCollider));

	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");

	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));

	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));

	weapon_->Initialize();

	weapon_->GetWorldTransform()->SetParent(GetRenderer("PlayerRightArm")->GetWorldTransform());
}

void Player::Update()
{
	if (currentState_) {
		currentState_->Update(*this);
	}

	GetWorldTransform()->GetTranslation() += velocity_ * DeltaTime::GetDeltaTime();
	velocity_ += acceleration_ * DeltaTime::GetDeltaTime();

	attackEfect_->Update();

	attackEfect_->UpdateAttackCylinderEffect(GetWorldTransform()->GetTranslation());
	attackEfect_->UpdateTargetMarkerEffect({ 0.0f, 0.0f, 5.0f });


	weapon_->Update();
	Object3d::Update();

	ImGui::Begin("Player");
	ImGui::DragFloat3("Translate", &GetWorldTransform()->GetTranslation().x, 0.01f);
	ImGui::DragFloat3("Rotate", &GetWorldTransform()->GetRotation().x, 0.01f);
	ImGui::DragFloat3("Scale", &GetWorldTransform()->GetScale().x, 0.01f);
	ImGui::End();
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
	if (other->category_ == CollisionCategory::Ground) {

		AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

		Vector3 playerMin = playerCollider->GetMin();
		Vector3 playerMax = playerCollider->GetMax();

		float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			velocity_.y = 0.0f;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionStay(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Ground) {

		AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

		Vector3 playerMin = playerCollider->GetMin();
		Vector3 playerMax = playerCollider->GetMax();

		float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			velocity_.y = 0.0f;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionExit(BaseCollider* other)
{
}
