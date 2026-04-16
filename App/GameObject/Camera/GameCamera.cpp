#include "GameCamera.h"
#include <3d/Object/Object3dManager.h>
#include "GameObject/Character/Player/Player.h"
#include <3d/Primitive/PrimitiveLineDrawer.h>
#include <Input/CameraInput.h>

GameCamera::GameCamera(std::string cameraName) : BaseCamera(cameraName)
{
	//horizontalAngle_ = 3.14f * 3.0f;
}

void GameCamera::Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput)
{
	player_ = player;
	lockOn_ = lockOn;
	cameraInput_ = cameraInput;
}

void GameCamera::Update()
{
    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    // ===== 入力 =====
    Vector2 stick = cameraInput_->GetStickDirection();

    yaw_ += stick.x * sensitivityX;
    pitch_ -= stick.y * sensitivityY;

    // Pitch制限
    float pitchLimit = 0.2f;
    pitch_ = std::clamp(pitch_, -pitchLimit, pitchLimit);

    float distance = 18.0f;
    float baseHeight = 3.0f;

    // ===== カメラ位置 =====
    Vector3 offset;
    offset.x = cos(pitch_) * sin(yaw_) * distance;
    offset.y = baseHeight + sin(pitch_) * distance;
    offset.z = cos(pitch_) * cos(yaw_) * distance;

    Vector3 desiredPos = playerPos + offset;

    // ===== 最小距離 =====
    float minDist = 5.0f;
    Vector3 toCamera = desiredPos - playerPos;
    if (Length(toCamera) < minDist)
    {
        desiredPos = playerPos + Normalize(toCamera) * minDist;
    }

    Vector3 forward = Normalize(player_->GetWorldTransform()->GetForward());
    float lookHeight = 2.0f;
    float lookOffset = 4.0f; // 視線の動き量

    Vector3 lookTarget = playerPos + Vector3(0, lookHeight, 0) + forward * 3.0f - Vector3(0, sin(pitch_) * lookOffset, 0);

    // ===== スムージング =====
    GetTranslate() = Lerp(GetTranslate(), desiredPos, 0.05f);

    LookAt(lookTarget);

    BaseCamera::Update();

    if (player_->IsLockOn()) {
        CameraManager::GetInstance()->SetActiveCamera("LockOnCamera", 0.3f);

        // GameCamera に渡す
        auto gameCam = static_cast<GameCamera*>(CameraManager::GetInstance()->FindCamera("GameCamera"));

        if (gameCam) {
            gameCam->SetYaw(yaw_);
        }
    }
}