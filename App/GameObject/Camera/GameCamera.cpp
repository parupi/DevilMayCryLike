#include "GameCamera.h"
#include <3d/Object/Object3dManager.h>
#include "GameObject/Character/Player/Player.h"

GameCamera::GameCamera(std::string cameraName) : Camera(cameraName)
{
    horizontalAngle_ = 3.14f * 3.0f;
}

void GameCamera::Update()
{
    if (!player_) {
        player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
        if (!player_) return; // プレイヤーがまだ見つからない場合は更新しない
    }

    // プレイヤーの位置を取得
    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    Vector3 cameraPos{};

    Vector3 lookTarget = playerPos;

    if (player_->IsLockOn()) {
        Vector3 lockOnPos = player_->GetLockOnPos();

        // プレイヤー → ロックオン対象のベクトル
        Vector3 toEnemy = lockOnPos - playerPos;
        toEnemy.y = 0.0f;
        toEnemy = Normalize(toEnemy);

        // 背後に回るために逆ベクトルを使用
        const float distance = 20.0f;
        const float height = 12.0f;

        cameraPos = playerPos - (toEnemy * distance);
        cameraPos.y += height;

        // プレイヤーとロックオン対象の中間点を見る
        lookTarget = (playerPos + lockOnPos) * 0.5f;
    } else {
        // === キー操作による水平角度の更新 ===
        const float angleSpeed = 0.02f; // 回転速度（ラジアン）
        if (input_->IsConnected()) {
            horizontalAngle_ += input_->GetRightStickX() * angleSpeed;
        } else {
            if (input_->PushKey(DIK_K)) {
                horizontalAngle_ -= angleSpeed;
            }
            if (input_->PushKey(DIK_L)) {
                horizontalAngle_ += angleSpeed;
            }
        }

        // 半径と高さを固定しつつ水平回転
        const float distance = 20.0f;  // プレイヤーからの距離
        const float height = 12.0f;    // 高さ

        Vector3 offset = {
            std::sin(horizontalAngle_) * distance,
            height,
            std::cos(horizontalAngle_) * distance,
        };

        // カメラの位置をプレイヤーの位置 + オフセット に設定
        cameraPos = playerPos + offset;

        // ======== ロックオン先がある場合は、プレイヤーとロックオンの中間点を見る ========
        lookTarget = playerPos;
       
    }
    
    GetTranslate() = cameraPos;
    LookAt(lookTarget);
    Camera::Update();
}
