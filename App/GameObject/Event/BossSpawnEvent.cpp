#include "BossSpawnEvent.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include "World3D/Camera/CameraManager.h"

BossSpawnEvent::BossSpawnEvent(std::string objectName)
	: BaseEvent(objectName, EventType::BossSpawn) {
	Object3d::Initialize();
}

void BossSpawnEvent::Initialize() {
	// トリガー専用。押し出し対象(Ground/Enemy)にならないよう明示的に None を設定しておく
	// （BaseCollider::category_ は未初期化のため必須）
	if (auto* col = GetCollider(name_)) {
		col->category_ = CollisionCategory::None;
	}
}

Enemy* BossSpawnEvent::FindBoss() const {
	if (bossName_.empty()) return nullptr;
	return dynamic_cast<Enemy*>(Object3dManager::GetInstance().FindObject(bossName_));
}

void BossSpawnEvent::Update(float deltaTime) {
	if (currentFrame_ < skipFrames_) {
		currentFrame_++;
	}

	if (phase_ == Phase::Playing) {
		timer_ += deltaTime;

		Enemy* boss = FindBoss();
		bool appearing = boss && boss->IsAppearanceEffectPlaying();

		// 出現が終わって見せ時間が経過したら終了（ボスが見つからない場合や保険の最大時間でも終了）
		bool holdElapsed = !appearing && timer_ >= kAppearDuration + kHoldTime;
		if (!boss || holdElapsed || timer_ >= kMaxCutsceneTime) {
			EndCutscene();
		}
	}

	Object3d::Update(deltaTime);
}

void BossSpawnEvent::OnCollisionEnter(BaseCollider* other) {
	if (currentFrame_ < skipFrames_) return; // 開始直後の誤発動を防ぐ
	if (isTriggered_) return;                // 一度きり

	if (other->category_ == CollisionCategory::Player) {
		Execute();
	}
}

void BossSpawnEvent::Execute() {
	isTriggered_ = true;

	Enemy* boss = FindBoss();
	if (!boss) {
		// ボスが設定されていない・見つからない場合は何もせず終了する
		phase_ = Phase::Finished;
		return;
	}

	player_ = static_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));

	// 演出中はプレイヤーをその場に固定する（移動範囲を現在位置に制限）
	if (player_) {
		const Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
		player_->SetMovementBounds(playerPos, playerPos);
	}

	// ボス出現開始（通常の敵よりゆっくり・濃い粒子で）
	if (auto* fx = boss->GetAppearanceFx()) {
		fx->SetAppearDuration(kAppearDuration);
		fx->SetAppearEmitCount(kAppearEmitCount);
	}
	boss->Spawn(); // 内部で地面へスナップされる

	// ── 演出カメラの生成 ──
	// ボスの正面（プレイヤー側）から少し横にずらした位置で、ボスの上半身を注視する
	const Vector3 bossPos = boss->GetWorldTransform()->GetTranslation(); // スナップ後の位置
	Vector3 toPlayer = { 0.0f, 0.0f, -1.0f };
	if (player_) {
		toPlayer = player_->GetWorldTransform()->GetTranslation() - bossPos;
		toPlayer.y = 0.0f;
		if (Length(toPlayer) > 0.001f) {
			toPlayer = Normalize(toPlayer);
		} else {
			toPlayer = { 0.0f, 0.0f, -1.0f };
		}
	}
	// toPlayer と直交する横方向（構図を少しずらすため）
	const Vector3 side = { -toPlayer.z, 0.0f, toPlayer.x };

	auto camera = std::make_unique<BaseCamera>(name_ + "Camera");
	camera->GetTranslate() = bossPos + toPlayer * kCameraDistance + side * kCameraSideOffset + Vector3{ 0.0f, kCameraHeight, 0.0f };
	camera->LookAt(bossPos + Vector3{ 0.0f, kLookAtHeight, 0.0f });
	CameraManager::GetInstance().AddCamera(std::move(camera));
	CameraManager::GetInstance().SetActiveCamera(name_ + "Camera", kCameraInTransition);

	phase_ = Phase::Playing;
	timer_ = 0.0f;
}

void BossSpawnEvent::EndCutscene() {
	CameraManager::GetInstance().SetActiveCamera("GameCamera", kCameraOutTransition);
	if (player_) {
		player_->ClearMovementBounds();
	}
	phase_ = Phase::Finished;
}
