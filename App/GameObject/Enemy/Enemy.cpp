#include "Enemy.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "GameObject/Player/Player.h"


Enemy::Enemy(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();



	
}

void Enemy::Initialize()
{
	

}

void Enemy::Update()
{
	if (!player_) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
	}

	if (currentState_) {
		currentState_->Update(*this);
	}


	GetWorldTransform()->GetTranslation() += velocity_ * DeltaTime::GetDeltaTime();
	velocity_ += acceleration_ * DeltaTime::GetDeltaTime();


	Object3d::Update();

	onGround_ = false;
}

void Enemy::Draw()
{


	Object3d::Draw();
}

void Enemy::DrawEffect()
{
	//effect_->Draw();
}

#ifdef _DEBUG

void Enemy::DebugGui()
{
	ImGui::Begin("Enemy");
	Object3d::DebugGui();
	ImGui::End();

	//ImGui::Begin("EnemyEffect");
	//effect_->DebugGui();
	//ImGui::End();
}
#endif // _DEBUG


void Enemy::OnCollisionEnter(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Ground) {

		AABBCollider* enemyCollider = static_cast<AABBCollider*>(GetCollider(name_));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(enemyCollider, blockCollider);

		Vector3 enemyMin = enemyCollider->GetMin();
		Vector3 enemyMax = enemyCollider->GetMax();

		float enemyOffset = (enemyMax.x - enemyMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + enemyOffset;
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - enemyOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + enemyOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			velocity_.y = 0.0f;
			onGround_ = true;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - enemyOffset;
			//velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + enemyOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - enemyOffset;
		}
	}
}

void Enemy::OnCollisionStay(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Ground) {

		AABBCollider* enemyCollider = static_cast<AABBCollider*>(GetCollider(name_));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);
		// 法線を取得
		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(enemyCollider, blockCollider);

		Vector3 enemyMin = enemyCollider->GetMin();
		Vector3 enemyMax = enemyCollider->GetMax();

		float enemyOffset = (enemyMax.x - enemyMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + enemyOffset;
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - enemyOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + enemyOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - enemyOffset;
			//velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + enemyOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - enemyOffset;
		}
	}
}

void Enemy::OnCollisionExit(BaseCollider* other)
{
	//if (other->category_ == CollisionCategory::PlayerWeapon) {
	//	
	//	hp_--;

	//	if (hp_ <= 0) {
	//		//GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };
	//	}
	//}
	other;
}

void Enemy::ChangeState(const std::string& stateName)
{
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}
