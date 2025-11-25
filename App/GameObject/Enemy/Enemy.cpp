#include "Enemy.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "GameObject/Player/Player.h"
#include <scene/Transition/TransitionManager.h>


Enemy::Enemy(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();


}

void Enemy::Initialize()
{
	currentState_ = states_["Air"].get();

}

void Enemy::Update()
{
	if (!player_) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
		GetCollider(name_)->category_ = CollisionCategory::Enemy;
	}

	// カメラ合切り替え中とフェード中は動かさない
	if (!TransitionManager::GetInstance()->IsFinished() || CameraManager::GetInstance()->IsTransition()) {
		Object3d::Update();
		return;
	}

	// 起動する前だったら動かない
	if (!isActive_) {
		return;
	}

	// 死んだときに関連する全コンポーネントを削除
	if (!isAlive_) {
		isAlive = false;
		GetCollider(name_)->isAlive = false;
		GetRenderer(name_)->isAlive = false;
		return;
	}

	slashEmitter_->Update();
	smokeEmitter_->Update();

	hitStop_->Update();
	if (hitStop_->GetHitStopData().isActive) {
		GetRenderer(name_)->GetWorldTransform()->GetTranslation() = hitStop_->GetHitStopData().translate;
		Object3d::Update();
		smokeEmitter_->Emit();
		return;
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
	if (isActive_ && isAlive_) {
		Object3d::Draw();
	}
}

void Enemy::DrawEffect()
{
}

#ifdef _DEBUG

void Enemy::DebugGui()
{
	ImGui::Begin("Enemy");
	Object3d::DebugGui();
	ImGui::End();
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

	if (other->category_ == CollisionCategory::PlayerWeapon) {

		hp_--;

		if (hp_ <= 0) {
			OnDeath();
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

void Enemy::OnDeath()
{
	isAlive_ = false;

}
