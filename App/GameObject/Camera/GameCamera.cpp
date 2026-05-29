#include "GameCamera.h"
#include "GameObject/Character/Player/Player.h"
#include <Input/CameraInput.h>
#include <base/utility/DeltaTime.h>
#include <cmath>

static constexpr float kCameraBaseHeight = 3.0f;

GameCamera::GameCamera(std::string cameraName)
    : BaseCamera(cameraName)
{
}

void GameCamera::Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput)
{
    player_ = player;
    lockOn_ = lockOn;
    cameraInput_ = cameraInput;
}

void GameCamera::SetMode(Mode mode)
{
    if (mode_ == mode) return;

    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
    Vector3 camPos = GetTranslate();

    // ===== LockOn → Free =====
    if (mode_ == Mode::LockOn && mode == Mode::Free)
    {
        Vector3 toCam = camPos - playerPos;

        float horizDist = std::sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
        float adjustedY  = toCam.y - kCameraBaseHeight;

        distance_ = std::sqrt(horizDist * horizDist + adjustedY * adjustedY);
        yaw_      = std::atan2(toCam.x, toCam.z);
        pitch_    = std::atan2(adjustedY, horizDist);

        // 注視オフセットをリセット：前方バイアスなしから滑らかに立ち上げる
        smoothedLookOffset_ = Vector3(0.0f, 0.0f, 0.0f);
    }

    // ===== Free → LockOn =====
    if (mode_ == Mode::Free && mode == Mode::LockOn)
    {
        // yawだけ引き継げばOK（見た目はそのまま）
    }

    mode_ = mode;
}

void GameCamera::Update()
{
    if (!player_) return;

    switch (mode_)
    {
    case Mode::Free:
        UpdateFree();
        break;

    case Mode::LockOn:
        UpdateLockOn();
        break;
    }

    BaseCamera::Update();
}

void GameCamera::UpdateFree()
{
    if (lockOn_->IsLockOn())
    {
        SetMode(Mode::LockOn);
        return;
    }

    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    Vector2 stick = cameraInput_->GetStickDirection();

    yaw_ += stick.x * sensitivityX;
    pitch_ -= stick.y * sensitivityY;

    float pitchLimit = 1.2f;
    pitch_ = std::clamp(pitch_, -pitchLimit, pitchLimit);

    Vector3 offset;
    offset.x = cos(pitch_) * sin(yaw_) * distance_;
    offset.y = kCameraBaseHeight + sin(pitch_) * distance_;
    offset.z = cos(pitch_) * cos(yaw_) * distance_;

    Vector3 desiredPos = playerPos + offset;

    float minDist = 5.0f;
    Vector3 toCamera = desiredPos - playerPos;
    if (Length(toCamera) < minDist)
    {
        desiredPos = playerPos + Normalize(toCamera) * minDist;
    }

    // プレイヤー前方への注視オフセットを遅延追従させる
    // ・k=2.5f で約0.27秒遅れ → 小さな動きを吸収しつつ持続した移動方向に追従
    Vector3 forward = Normalize(player_->GetWorldTransform()->GetForward());
    float lookT = 1.0f - std::exp(-1.0f * DeltaTime::GetDeltaTime());
    smoothedLookOffset_ = Lerp(smoothedLookOffset_, forward * 3.0f, lookT);

    Vector3 lookTarget = playerPos + Vector3(0, 2.0f, 0) + smoothedLookOffset_ - Vector3(0, sin(pitch_) * 4.0f, 0);

    float t = 1.0f - std::exp(-5.0f * DeltaTime::GetDeltaTime());
    GetTranslate() = Lerp(GetTranslate(), desiredPos, t);

    LookAt(lookTarget);
}

void GameCamera::UpdateLockOn()
{
    if (!lockOn_->IsLockOn())
    {
        SetMode(Mode::Free);
        return;
    }

    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
    Vector3 enemyPos = lockOn_->GetCurrentTarget()->GetWorldPosition();

    Vector3 toEnemy = enemyPos - playerPos;
    toEnemy.y = 0.0f;
    toEnemy = Normalize(toEnemy);

    float distToEnemy = Length(enemyPos - playerPos);

    float distance = std::clamp(distToEnemy * 1.2f, 12.0f, 25.0f);

    const float height = 8.0f;

    Vector3 right = Normalize(Cross(Vector3(0, 1, 0), toEnemy));

    Vector3 cameraPos = playerPos - toEnemy * distance + right * 3.0f;

    cameraPos.y += height;

    Vector3 lookTarget = (playerPos + enemyPos) * 0.5f + Vector3(0, 2.0f, 0);

    float t = 1.0f - std::exp(-5.0f * DeltaTime::GetDeltaTime());
    GetTranslate() = Lerp(GetTranslate(), cameraPos, t);

    LookAt(lookTarget);

    // Free復帰用
    yaw_ = std::atan2(toEnemy.x, toEnemy.z);
}