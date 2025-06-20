#include "GameCamera.h"
#include <Object3dManager.h>
#include "GameObject/Player/Player.h"

GameCamera::GameCamera(std::string cameraName) : Camera(cameraName)
{
	
}

void GameCamera::Update()
{
    if (!player_) {
        player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
        if (!player_) return; // プレイヤーがまだ見つからない場合は更新しない
    }

    // プレイヤーの位置を取得
    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    // カメラのオフセット（プレイヤーの後方上に配置）
    Vector3 offset = { 0.0f, 12.0f, -20.0f };

    // カメラの位置をプレイヤーの位置 + オフセット に設定
    Vector3 cameraPos = playerPos + offset;
    GetTranslate() = cameraPos;

    // プレイヤーの位置を見るようにカメラの注視点を設定
    LookAt(playerPos);

    // カメラの更新（親クラスの Update を呼ぶ）
    Camera::Update();
}
