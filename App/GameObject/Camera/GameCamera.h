#pragma once
#include "World3D/Camera/BaseCamera.h"

class Player;
class LockOnSystem;
class CameraInput;

class GameCamera : public BaseCamera {
public:
	enum class Mode {
		Free,
		LockOn
	};

	GameCamera(std::string cameraName);
	~GameCamera() override = default;

	void Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput);
	void Update() override;

	// モードの切り替え
	void SetMode(Mode mode);

private:
	void UpdateFree();
	void UpdateLockOn();
	// 注視点を滑らかに追従させてからLookAtする（モード切替時の視点飛びを防ぐ）
	void ApplySmoothLookAt(const Vector3& lookTarget);

private:
	Player* player_ = nullptr;
	LockOnSystem* lockOn_ = nullptr;
	CameraInput* cameraInput_ = nullptr;

	Mode mode_ = Mode::Free;

	float yaw_ = 3.14f;
	float pitch_ = 0.0f;
	float distance_ = 18.0f;

	Vector3 smoothedLookOffset_ = Vector3(0.0f, 0.0f, 0.0f);

	Vector3 smoothedLookTarget_ = Vector3(0.0f, 0.0f, 0.0f);
	bool lookTargetInitialized_ = false;

	Vector3 velocity_ = Vector3(0.0f, 0.0f, 0.0f);

	float sensitivityX = 0.03f;
	float sensitivityY = 0.025f;
};