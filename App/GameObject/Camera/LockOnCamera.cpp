#include "LockOnCamera.h"
#include <3d/Object/Object3dManager.h>
#include "GameObject/Character/Player/Player.h"
#include "GameCamera.h"

LockOnCamera::LockOnCamera(std::string cameraName) : BaseCamera(cameraName)
{

}

void LockOnCamera::Update()
{
    if (!player_) {
        player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
        if (!player_) return; // プレイヤーがまだ見つからない場合は更新しない
    }

    // プレイヤーの位置を取得
    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    Vector3 cameraPos{};

    Vector3 lookTarget = playerPos;

    Vector3 lockOnPos = player_->GetLockOnPos();
    if (lockOnPos.x == 0.0f && lockOnPos.y == 0.0f && lockOnPos.z == 0.0f) {
        CameraManager::GetInstance()->SetActiveCamera("GameCamera", 0.3f);
        return;
    }

    // プレイヤー → ロックオン対象のベクトル
    Vector3 toEnemy = lockOnPos - playerPos;
    toEnemy.y = 0.0f;
    toEnemy = Normalize(toEnemy);

    const float distance = 18.0f;
    const float height = 8.0f;

    cameraPos = playerPos - (toEnemy * distance);
    cameraPos.y += height;

    // プレイヤーとロックオン対象の中間点を見る
    lookTarget = (playerPos + lockOnPos) * 0.5f;

    GetTranslate() = cameraPos;
    LookAt(lookTarget);
    BaseCamera::Update();

    Vector3 camToPlayer = GetTranslate() - playerPos;
    camToPlayer.y = 0.0f;

    camToPlayer = Normalize(camToPlayer);

    float horizontalAngle = std::atan2(camToPlayer.x, camToPlayer.z);

    if (player_) {
        if (!player_->IsLockOn()) {

            // 今のロックオンカメラの角度を計算
            Vector3 camToPlayer = GetTranslate() - playerPos;
            camToPlayer.y = 0.0f;
            camToPlayer = Normalize(camToPlayer);

            float angle = std::atan2(camToPlayer.x, camToPlayer.z);

            // GameCamera に渡す
            auto gameCam = static_cast<GameCamera*>(CameraManager::GetInstance()->FindCamera("GameCamera"));

            if (gameCam) {
                gameCam->SetHorizontalAngle(angle);
            }

            CameraManager::GetInstance()->SetActiveCamera("GameCamera", 0.2f);
        }
    }
}
