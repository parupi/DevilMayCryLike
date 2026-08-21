#include "EnemySpawnEvent.h"

EnemySpawnEvent::EnemySpawnEvent(std::string objectName) : BaseEvent(objectName, EventType::EnemySpawn) {
	Object3d::Initialize();
}


void EnemySpawnEvent::AddEnemy(Enemy* enemy) {
	enemies_.push_back(enemy);
}

void EnemySpawnEvent::Initialize() {
	// 進入を見るだけのトリガー。押し出し対象(Ground/Enemy)にならないよう明示しておく
	for (BaseCollider* collider : GetColliders()) {
		collider->category_ = CollisionCategory::None;
	}
}

void EnemySpawnEvent::Update(float deltaTime) {
	if (currentFrame_ < skipFrames_) {
		currentFrame_++;
	}


	Object3d::Update(deltaTime);
}


void EnemySpawnEvent::Execute() {
	isTriggered_ = true;
	for (auto& enemy : enemies_) {
		enemy->Spawn();
	}
}

void EnemySpawnEvent::OnCollisionEnter(BaseCollider* other) {
	if (currentFrame_ < skipFrames_) {
		return; // 最初の数フレームは処理しない
	}

	if (other->category_ == CollisionCategory::Player) {
		if (!isTriggered_) {
			Execute();
		}
	}
}
