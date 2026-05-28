#include "LockOnCamera.h"
#include <3d/Object/Object3dManager.h>
#include "GameObject/Character/Player/Player.h"
#include "GameCamera.h"
#include <base/utility/DeltaTime.h>
#include <cmath>

LockOnCamera::LockOnCamera(std::string cameraName) : BaseCamera(cameraName)
{

}

void LockOnCamera::Initialize(Player* player, LockOnSystem* lockOn)
{
	player_ = player;
	lockOn_ = lockOn;
}

void LockOnCamera::Update()
{
	if (!player_) return;
	// ロックオン状態の取得
	bool isLockOn = lockOn_->IsLockOn();

	// プレイヤーの位置を取得
	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
	// ロックオンしていない場合は通常のカメラに切り替える
	if (!isLockOn) {
		// 今のロックオンカメラの角度を計算
		Vector3 camToPlayer = GetTranslate() - playerPos;
		camToPlayer.y = 0.0f;
		camToPlayer = Normalize(camToPlayer);

		float angle = std::atan2(camToPlayer.x, camToPlayer.z);

		// GameCamera に渡す
		auto gameCam = static_cast<GameCamera*>(CameraManager::GetInstance()->FindCamera("GameCamera"));

		if (gameCam) {
			//gameCam->SetYaw(angle);
		}

		CameraManager::GetInstance()->SetActiveCamera("GameCamera", 0.3f);
		return;
	}

	Vector3 lockOnPos = lockOn_->GetCurrentTarget()->GetWorldPosition();

	Vector3 toEnemy;
	if (isLockOn && !wasLockOn_) {
		// 初期のフレームだけGameCameraの向きを引き継ぐ
		toEnemy.x = sin(yaw_);
		toEnemy.y = 0.0f;
		toEnemy.z = cos(yaw_);
	}
	else {
		// プレイヤー → ロックオン対象のベクトル
		toEnemy = lockOnPos - playerPos;
		toEnemy.y = 0.0f;
		
	}
	toEnemy = Normalize(toEnemy);

	float distToEnemy = Length(lockOnPos - playerPos);

	// 距離に応じてカメラを引く
	float distance = std::clamp(distToEnemy * 1.2f, 12.0f, 25.0f);

	const float height = 8.0f;

	// 横方向を作る
	Vector3 right = Normalize(Cross(Vector3(0, 1, 0), toEnemy));
	// 少し横にずらす
	float sideOffset = 3.0f;
	Vector3 cameraPos = playerPos - toEnemy * distance + right * sideOffset;
	cameraPos.y += height;

	// 少し上を見る
	Vector3 lookTarget = (playerPos + lockOnPos) * 0.5f + Vector3(0, 2.0f, 0);

	float t = 1.0f - std::exp(-5.0f * DeltaTime::GetDeltaTime());
	GetTranslate() = Lerp(GetTranslate(), cameraPos, t);

	LookAt(lookTarget);

	BaseCamera::Update();
	// 状態の更新
	wasLockOn_ = isLockOn;
}
