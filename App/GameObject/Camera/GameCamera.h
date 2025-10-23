#pragma once
#include "3d/Camera/Camera.h"
#include "input/Input.h"

class Player;

/// <summary>
/// ゲーム中のプレイヤーを追従するカメラを管理するクラス
/// プレイヤーの位置と入力に基づいてカメラを制御する
/// </summary>
class GameCamera : public Camera
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

	/// <summary>
	/// プレイヤーを追従するカメラの更新処理
	/// 入力による回転・追従などの挙動を更新する
	/// </summary>
	void Update() override;

private:
	/// <summary>
	/// 追従対象のプレイヤー
	/// </summary>
	Player* player_ = nullptr;

	/// <summary>
	/// 入力管理クラスのインスタンス
	/// </summary>
	Input* input_ = Input::GetInstance();

	/// <summary>
	/// 左右回転角（ラジアン）
	/// </summary>
	float horizontalAngle_ = 0.0f;
};
