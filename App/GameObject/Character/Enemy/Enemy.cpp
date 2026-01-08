#include "Enemy.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "GameObject/Character/Player/Player.h"
#include <scene/Transition/TransitionManager.h>
#include "BaseState/EnemyStateKnockBack.h"


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


	if (currentState_) {
		currentState_->Update(*this);
	}

	if (!hitStop_->GetHitStopData().isActive) {
		GetWorldTransform()->GetTranslation() += velocity_ * DeltaTime::GetDeltaTime();
		velocity_ += acceleration_ * DeltaTime::GetDeltaTime();
	}

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
		HitDamage();

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
	other;
}

void Enemy::ChangeState(const std::string& stateName, const DamageInfo* info)
{
	if (currentState_) currentState_->Exit(*this);

	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();

		// KnockBack系ならDamageInfoを渡す
		if (info) {
			if (EnemyStateKnockBack* s = dynamic_cast<EnemyStateKnockBack*>(currentState_)) {
				s->Enter(*info, *this);
			}
		} else {
			currentState_->Enter(*this);
		}
	}
}

void Enemy::OnDeath()
{
	isAlive_ = false;

}

void Enemy::HitDamage()
{
	if (!player_) return;

	// ノックバックに使うパラメータを全てプレイヤーの攻撃から取得
	damageInfo_.damage = player_->GetAttackData().damage;
	damageInfo_.hitPosition = GetWorldTransform()->GetTranslation();
	damageInfo_.hitNormal = { 0.0f, 0.0f, 0.0f };
	damageInfo_.attackerPosition = player_->GetWorldTransform()->GetTranslation();
	damageInfo_.direction = Normalize(damageInfo_.hitPosition - damageInfo_.attackerPosition);

	damageInfo_.type = player_->GetAttackData().type;

	damageInfo_.impulseForce = player_->GetAttackData().impulseForce;
	damageInfo_.upwardRatio = player_->GetAttackData().upwardRatio;
	damageInfo_.torqueForce = player_->GetAttackData().torqueForce;
	damageInfo_.stunTime = player_->GetAttackData().stunTime;

	//hp_ -= damageInfo_.damage;

	//if (hp_ <= 0) {
	//	OnDeath();
	//}
	// TODO : 敵ごとにノックバックステートを用意する
	ChangeState("KnockBack", &damageInfo_);
}
