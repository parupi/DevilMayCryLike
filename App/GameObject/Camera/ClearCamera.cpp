#include "ClearCamera.h"

ClearCamera::ClearCamera(std::string cameraName) : Camera(cameraName)
{
    
}

void ClearCamera::Update()
{
    if (!player_) {
        player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
        if (!player_) return;
    }

    // プレイヤーの位置
    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    // プレイヤーの向き（前方向）
    Vector3 forward = player_->GetWorldTransform()->GetForward();

    // 横方向（Right）
    Vector3 right = Normalize(Cross({ 0,1,0 }, forward));
    // Cross(up, forward) = プレイヤーから見て右方向

    // オフセット（近く＆真横）
    const float sideDistance = 6.0f;   // 横距離（近め）
    const float height = 3.0f;         // カメラの高さ
    const float forwardOffset = 1.0f;  // 少し前にずらすと見やすい

    Vector3 cameraPos = playerPos
        + right * sideDistance       // 真横
        + Vector3{ 0, height, 0 }      // 高さ
    - forward * forwardOffset;   // 少し後方に引く

    // カメラが見る位置（顔付近）
    Vector3 lookTarget = playerPos + Vector3{ 0, 1.5f, 0 };

    // 設定
    GetTranslate() = cameraPos;
    LookAt(lookTarget);

    Camera::Update();
}