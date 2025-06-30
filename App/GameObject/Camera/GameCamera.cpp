#include "GameCamera.h"
#include <3d/Object/Object3dManager.h>
#include "GameObject/Player/Player.h"

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

    // === キー操作による水平角度の更新 ===
    const float angleSpeed = 0.02f; // 回転速度（ラジアン）
    if (Input::GetInstance()->PushKey(DIK_K)) {
        horizontalAngle_ -= angleSpeed;
    }
    if (Input::GetInstance()->PushKey(DIK_L)) {
        horizontalAngle_ += angleSpeed;
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
    Vector3 cameraPos = playerPos + offset;
    GetTranslate() = cameraPos;

    // プレイヤーの位置を見るようにカメラの注視点を設定
    LookAt(playerPos);

    // カメラの更新（親クラスの Update を呼ぶ）
    Camera::Update();

    ImGui::Begin("Camera");
    ImGui::DragFloat3("Translate", &offset.x, 0.01f);
    ImGui::DragFloat3("Rotate", &transform_.rotate.x, 0.01f);
    ImGui::DragFloat("angle", &horizontalAngle_, 0.01f);
    ImGui::End();

    PrintOnImGui(GetForward(), "forward");
}
