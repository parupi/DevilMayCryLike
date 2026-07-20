#include "ForceBattleEvent.h"
#include "GameObject/Character/Player/Player.h"
#include <World3D/Collider/OBBCollider.h>

ForceBattleEvent::ForceBattleEvent(std::string objectName)
	: BaseEvent(objectName, EventType::ForceBattle) {
	Object3d::Initialize();
}

void ForceBattleEvent::Initialize() {
	// エリア兼トリガー。押し出し対象(Ground/Enemy)にならない純粋なトリガーにする。
	// （BaseCollider::category_ は未初期化のため、明示的に None を設定しておく）
	if (auto* col = GetCollider(name_)) {
		col->category_ = CollisionCategory::None;
	}
}

void ForceBattleEvent::AddEnemy(Enemy* enemy) {
	if (enemy) {
		enemyNames_.push_back(enemy->name_);
	}
}

void ForceBattleEvent::Update(float deltaTime) {
	if (currentFrame_ < skipFrames_) {
		currentFrame_++;
	}

	// 戦闘中は全滅を監視し、全滅したらプレイヤーのロックを解除する
	if (isBattleActive_ && AreAllEnemiesDefeated()) {
		EndBattle();
	}

	Object3d::Update(deltaTime);
}

void ForceBattleEvent::OnCollisionEnter(BaseCollider* other) {
	if (currentFrame_ < skipFrames_) return; // 開始直後の誤発動を防ぐ
	if (isTriggered_) return;                // 一度きり

	if (other->category_ == CollisionCategory::Player) {
		Execute();
	}
}

void ForceBattleEvent::Execute() {
	isTriggered_ = true;
	isBattleActive_ = true;

	// 関連付けられた敵を出現させる
	for (const auto& enemyName : enemyNames_) {
		if (auto* obj = Object3dManager::GetInstance().FindObject(enemyName)) {
			static_cast<Enemy*>(obj)->Spawn();
		}
	}

	// プレイヤーをエリア範囲内に閉じ込める（エリアはこのイベントのコライダー範囲）
	player_ = static_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));
	if (player_) {
		auto* col = dynamic_cast<OBBCollider*>(GetCollider(name_));
		if (col) {
			const Vector3 c = col->GetCenter();
			const Vector3 h = col->GetWorldHalfExtents();
			const Vector3 min = {c.x - h.x, c.y - h.y, c.z - h.z};
			const Vector3 max = {c.x + h.x, c.y + h.y, c.z + h.z};
			player_->SetMovementBounds(min, max);
		}
	}
}

void ForceBattleEvent::EndBattle() {
	isBattleActive_ = false;
	isFinished_ = true;
	if (player_) {
		player_->ClearMovementBounds();
	}
}

bool ForceBattleEvent::AreAllEnemiesDefeated() const {
	for (const auto& enemyName : enemyNames_) {
		// 削除済み（FindObjectで見つからない）＝撃破済み
		auto* obj = Object3dManager::GetInstance().FindObject(enemyName);
		if (obj && static_cast<Enemy*>(obj)->IsAlive()) {
			return false;
		}
	}
	return true;
}
