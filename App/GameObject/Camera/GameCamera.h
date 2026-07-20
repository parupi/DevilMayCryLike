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

	/// <summary>
	/// 追従位置（プレイヤーの背後）へ補間なしで即座に移動する。
	/// CameraManager はカメラ切り替えの補間先を SetActiveCamera を呼んだ時点の位置で記録するため、
	/// スタート演出の「引き」の到達点を正しくするには、切り替え前にこれを呼んでおく必要がある。
	/// </summary>
	/// <param name="playerForward">プレイヤーの向き（水平・単位ベクトル）</param>
	void SnapToFollow(const Vector3& playerForward);

private:
	void UpdateFree();
	void UpdateLockOn();
	// yaw/pitch/distance から、プレイヤーに対する追従位置を求める
	Vector3 CalcDesiredPosition(const Vector3& playerPos) const;
	// 注視点を滑らかに追従させてからLookAtする（モード切替時の視点飛びを防ぐ）
	void ApplySmoothLookAt(const Vector3& lookTarget);
	// ピボット→希望位置の間にGroundコライダー（壁・床）があれば、カメラを遮蔽物の手前へ引き寄せる
	Vector3 ResolveCameraCollision(const Vector3& pivot, const Vector3& desiredPos) const;

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