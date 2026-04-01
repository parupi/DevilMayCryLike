#pragma once
#include "3d/Camera/BaseCamera.h"
#include <input/Input.h>

class Player;
class LockOnSystem;

class LockOnCamera : public BaseCamera
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="cameraName">カメラの名前</param>
	LockOnCamera(std::string cameraName);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~LockOnCamera() override = default;

	// 初期化処理
	// シーンからプレイヤーとロックオンを受け取る
	void Initialize(Player* player, LockOnSystem* lockOn);

	/// <summary>
	/// プレイヤーを追従するカメラの更新処理
	/// 入力による回転・追従などの挙動を更新する
	/// </summary>
	void Update() override;

private:
	// 追従対象のプレイヤー
	Player* player_ = nullptr;
	// ロックオン対象を選別するクラス
	LockOnSystem* lockOn_ = nullptr;

	/// <summary>
	/// 入力管理クラスのインスタンス
	/// </summary>
	Input* input_ = Input::GetInstance();

	/// <summary>
	/// 左右回転角（ラジアン）
	/// </summary>
	float horizontalAngle_ = 0.0f;
};

