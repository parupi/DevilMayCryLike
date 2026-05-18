#pragma once
#include "3d/Camera/BaseCamera.h"
#include "input/Input.h"

class Player;
class LockOnSystem;
class CameraInput;

class GameCamera : public BaseCamera
{
public:
    enum class Mode
    {
        Free,
        LockOn
    };

    GameCamera(std::string cameraName);
    ~GameCamera() override = default;

    void Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput);
    void Update() override;

    // ★ 修正：関数化
    void SetMode(Mode mode);

private:
    void UpdateFree();
    void UpdateLockOn();

private:
    Player* player_ = nullptr;
    LockOnSystem* lockOn_ = nullptr;
    CameraInput* cameraInput_ = nullptr;

    Mode mode_ = Mode::Free;

    float yaw_ = 3.14f;
    float pitch_ = 0.0f;
    float distance_ = 18.0f;

    Vector3 velocity_ = Vector3(0.0f, 0.0f, 0.0f);

    float sensitivityX = 0.03f;
    float sensitivityY = 0.01f;
};