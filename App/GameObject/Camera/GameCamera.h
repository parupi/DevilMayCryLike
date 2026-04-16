#pragma once
#include "3d/Camera/BaseCamera.h"
#include "input/Input.h"


class Player;
class LockOnSystem;
class CameraInput;

/// <summary>
/// ゲーム中のプレイヤーを追従するカメラを管理するクラス
/// プレイヤーの位置と入力に基づいてカメラを制御する
/// </summary>
class GameCamera : public BaseCamera
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="cameraName">カメラの名前</param>
	GameCamera(std::string cameraName);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~GameCamera() override = default;

	// 初期化処理
	void Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput);

	/// <summary>
	/// プレイヤーを追従するカメラの更新処理
	/// 入力による回転・追従などの挙動を更新する
	/// </summary>
	void Update() override;

	void SetYaw(float yaw) { yaw_ = yaw; }
private:
	// 追従対象のプレイヤー
	Player* player_ = nullptr;
	// ロックオン対象を選別するクラス
	LockOnSystem* lockOn_ = nullptr;
	// カメラの入力を受け取るクラス
	CameraInput* cameraInput_ = nullptr;
	// 左右
	float yaw_ = 3.14f;
	// 上下
	float pitch_ = 0.0f;
	// カメラの回転感度
	float sensitivityX = 0.03f;
	float sensitivityY = 0.01f;
};
