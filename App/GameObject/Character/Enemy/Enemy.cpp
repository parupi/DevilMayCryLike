#include "Enemy.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "GameObject/Character/Player/Player.h"
#include <scene/Transition/TransitionManager.h>


Enemy::Enemy(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();

}

Enemy::~Enemy()
{
	lockOnTarget_.Finalize();
}

void Enemy::Initialize()
{
	if (!hitStop_) {
		hitStop_ = std::make_unique<HitStop>();
	}

	auto it = states_.find("Air");
	if (it != states_.end()) {
		currentState_ = it->second.get();
	}
}

void Enemy::Update(float deltaTime)
{
	if (!player_) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
		GetCollider(name_)->category_ = CollisionCategory::Enemy;
	}

	// 起動する前だったら動かない
	if (!isActive_) {
		return;
	}

	if (!isAlive_) {
		isAlive = false;
		GetCollider(name_)->isAlive = false;
		GetRenderer(name_)->isAlive = false;
		return;
	}

	hitStop_->Update(deltaTime);
	float dt = deltaTime * hitStop_->GetTimeScale();

	if (currentState_) {
		currentState_->Update(*this, dt);
	}

	GetWorldTransform()->GetTranslation() += velocity_ * dt;
	velocity_ += acceleration_ * dt;

	if (!player_) {
		return;
	}

	// 座標取得
	Vector3 enemyPos = GetWorldTransform()->GetTranslation();
	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

	// 敵 → プレイヤー方向
	Vector3 dir = enemyPos - playerPos;
	dir.y = 0.0f; // 上下は無視
	Normalize(dir);

	// 前方向（モデルの前が +Z 前提）
	Vector3 forward(0.0f, 0.0f, 1.0f);

	// 回転Quaternionを計算
	Quaternion rot = FromToRotation(forward, dir);

	// 回転をセット
	GetWorldTransform()->GetRotation() = rot;

	Object3d::Update(dt);

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

void Enemy::Spawn() {
	

	Object3d::Update(0.0f);
	isActive_ = true;
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
	if (other->category_ != CollisionCategory::Ground) return;

	AABBCollider* enemyCollider = static_cast<AABBCollider*>(GetCollider(name_));
	AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

	Vector3 outNormal = AABBCollider::CalculateCollisionNormal(enemyCollider, blockCollider);
	float enemyOffset = (enemyCollider->GetMax().x - enemyCollider->GetMin().x) * 0.5f;

	if (outNormal.x == 1.0f) {
		GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + enemyOffset;
	} else if (outNormal.x == -1.0f) {
		GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - enemyOffset;
	} else if (outNormal.y == 1.0f) {
		GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + enemyOffset - 0.1f;
		velocity_.y = 0.0f;
		onGround_ = true;
	} else if (outNormal.y == -1.0f) {
		GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - enemyOffset;
	} else if (outNormal.z == 1.0f) {
		GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + enemyOffset;
	} else if (outNormal.z == -1.0f) {
		GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - enemyOffset;
	}
}

void Enemy::OnCollisionStay(BaseCollider* other)
{
	if (other->category_ != CollisionCategory::Ground) return;

	AABBCollider* enemyCollider = static_cast<AABBCollider*>(GetCollider(name_));
	AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

	Vector3 outNormal = AABBCollider::CalculateCollisionNormal(enemyCollider, blockCollider);
	float enemyOffset = (enemyCollider->GetMax().x - enemyCollider->GetMin().x) * 0.5f;

	if (outNormal.x == 1.0f) {
		GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + enemyOffset;
	} else if (outNormal.x == -1.0f) {
		GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - enemyOffset;
	} else if (outNormal.y == 1.0f) {
		GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + enemyOffset - 0.1f;
		onGround_ = true;
	} else if (outNormal.y == -1.0f) {
		GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - enemyOffset;
	} else if (outNormal.z == 1.0f) {
		GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + enemyOffset;
	} else if (outNormal.z == -1.0f) {
		GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - enemyOffset;
	}
}

void Enemy::OnCollisionExit(BaseCollider* other)
{
	other;
}

void Enemy::ChangeState(const std::string& stateName)
{
	if (currentState_) currentState_->Exit(*this);

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

void Enemy::SetupLockOn(LockOnSystem* lockOnSystem)
{
	lockOnTarget_.Initialize(lockOnSystem, this);
}

