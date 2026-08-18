#include "EnemySpawner.h"
#include "Enemy.h"

#include <Scene/Object3dFactory.h>
#include <Utility/Logger.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Collider/OBBCollider.h>
#include <World3D/Object/Object3dManager.h>

Enemy* EnemySpawner::Spawn(const std::string& className, const std::string& objectName,
	const Vector3& position, const Vector3& colliderHalfExtents)
{
	auto object = Object3dFactory::Create(className, objectName);
	// 未登録のクラス名を渡すと素の Object3d が返ってくる。敵でなければ何も足さずに帰る
	Enemy* enemy = dynamic_cast<Enemy*>(object.get());
	if (!enemy) {
		Logger::Log("[EnemySpawner] " + className + " は敵クラスとして登録されていません\n");
		return nullptr;
	}

	// 本体コライダー。カテゴリは Enemy::Update が最初のフレームで Enemy に設定する。
	// ここで付けておかないと BossKnight::Initialize() が null 参照で落ちる
	auto collider = std::make_unique<OBBCollider>(objectName);
	OBBData data;
	data.halfExtents = colliderHalfExtents;
	collider->GetColliderData() = data;
	BaseCollider* rawCollider = collider.get();
	CollisionManager::GetInstance().AddCollider(std::move(collider));
	enemy->AddCollider(rawCollider);

	// Object3d::Initialize() は冪等なので、先に位置を入れても消えない
	enemy->GetWorldTransform()->GetTranslation() = position;

	enemy->Initialize();

	// ステージへ保存する対象にはしない（実行中に増えたものが焼き付いてしまう）
	enemy->SetStageObject(false);

	Object3dManager::GetInstance().AddObject(std::move(object));
	return enemy;
}
