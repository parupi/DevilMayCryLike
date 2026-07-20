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

	// 壁はプレイヤーの近くだけを光らせるので、毎フレーム現在位置を渡す
	if (player_) {
		areaWall_.Update(deltaTime, player_->GetWorldTransform()->GetWorldPos());
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

	// プレイヤーをエリア範囲内に閉じ込める（エリアはこのイベントのコライダー範囲）
	player_ = static_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));

	MovementBounds bounds{};
	const bool hasBounds = TryGetAreaBounds(bounds);

	// 敵にはエリアより少し内側を歩かせる。境界ぴったりだと体が壁にめり込んで
	// 「外に出ている」ように見えてしまうため
	const MovementBounds enemyBounds = bounds.Shrunk(kEnemyBoundsMargin);

	// 関連付けられた敵を出現させ、エリア内に閉じ込める
	for (const auto& enemyName : enemyNames_) {
		if (auto* obj = Object3dManager::GetInstance().FindObject(enemyName)) {
			auto* enemy = static_cast<Enemy*>(obj);
			enemy->Spawn();
			if (hasBounds) {
				enemy->SetMovementBounds(enemyBounds);
			}
		}
	}

	if (hasBounds) {
		if (player_) {
			player_->SetMovementBounds(bounds);
		}
		// どこまで動けるのかが分かるよう、閉じ込めた範囲を格子状の壁で可視化する
		areaWall_.Initialize(bounds);
		areaWall_.Start();
	}
}

bool ForceBattleEvent::TryGetAreaBounds(MovementBounds& outBounds) {
	auto* col = dynamic_cast<OBBCollider*>(GetCollider(name_));
	if (!col) return false;

	const Vector3 c = col->GetCenter();
	const Vector3 h = col->GetWorldHalfExtents();

	// エリアは床の向きに合わせて鉛直軸まわりに回してあるので、
	// コライダーのローカル軸をそのまま水平2軸として使う。
	// （軸0/軸2を水平面に落として正規化する。鉛直軸まわりの回転だけなら向きは変わらない）
	auto flatten = [](const Vector3& axis, const Vector3& fallback) {
		Vector3 flat = { axis.x, 0.0f, axis.z };
		return (Length(flat) > 1e-4f) ? Normalize(flat) : fallback;
		};

	outBounds.center = c;
	outBounds.axisU = flatten(col->GetAxis(0), { 1.0f, 0.0f, 0.0f });
	outBounds.axisV = flatten(col->GetAxis(2), { 0.0f, 0.0f, 1.0f });
	outBounds.halfU = h.x;
	outBounds.halfV = h.z;
	outBounds.minY = c.y - h.y;
	outBounds.maxY = c.y + h.y;
	return true;
}

void ForceBattleEvent::EndBattle() {
	isBattleActive_ = false;
	isFinished_ = true;
	areaWall_.Stop();
	if (player_) {
		player_->ClearMovementBounds();
	}

	// 生き残っている敵（撃破しきる前に戦闘が終わる場合）の制限も解除しておく
	for (const auto& enemyName : enemyNames_) {
		if (auto* obj = Object3dManager::GetInstance().FindObject(enemyName)) {
			static_cast<Enemy*>(obj)->ClearMovementBounds();
		}
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
