#include "GameCamera.h"
#include "GameObject/Character/Player/Player.h"
#include <Input/CameraInput.h>

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

        distance_ = Length(toCam);

        if (distance_ > 0.001f)
        {
            yaw_ = std::atan2(toCam.x, toCam.z);
            pitch_ = std::asin(toCam.y / distance_);
        }
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

    float pitchLimit = 0.2f;
    pitch_ = std::clamp(pitch_, -pitchLimit, pitchLimit);

    float baseHeight = 3.0f;

    Vector3 offset;
    offset.x = cos(pitch_) * sin(yaw_) * distance_;
    offset.y = baseHeight + sin(pitch_) * distance_;
    offset.z = cos(pitch_) * cos(yaw_) * distance_;

    Vector3 desiredPos = playerPos + offset;

    float minDist = 5.0f;
    Vector3 toCamera = desiredPos - playerPos;
    if (Length(toCamera) < minDist)
    {
        desiredPos = playerPos + Normalize(toCamera) * minDist;
    }

    Vector3 forward = Normalize(player_->GetWorldTransform()->GetForward());

    Vector3 lookTarget =
        playerPos + Vector3(0, 2.0f, 0) + forward * 3.0f
        - Vector3(0, sin(pitch_) * 4.0f, 0);

    GetTranslate() = Lerp(GetTranslate(), desiredPos, 0.05f);

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

    Vector3 cameraPos =
        playerPos - toEnemy * distance
        + right * 3.0f;

    cameraPos.y += height;

    Vector3 lookTarget =
        (playerPos + enemyPos) * 0.5f
        + Vector3(0, 2.0f, 0);

    GetTranslate() = Lerp(GetTranslate(), cameraPos, 0.05f);

    LookAt(lookTarget);

    // Free復帰用
    yaw_ = std::atan2(toEnemy.x, toEnemy.z);
}