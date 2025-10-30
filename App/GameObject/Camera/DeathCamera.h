#pragma once
#include <3d/Camera/Camera.h>
#include <math/Vector3.h>
#include <random>

class Player;

class DeathCamera : public Camera
{
public:
	DeathCamera(const std::string& cameraName, Camera* sourceCamera, Player* player);
	~DeathCamera() override = default;

	/// <summary>
	/// プレイヤーを追従するカメラの更新処理
	/// 入力による回転・追従などの挙動を更新する
	/// </summary>
	void Update() override;

private:
	Player* player_ = nullptr;

	Vector3 basePos_;        // ゲームカメラの初期位置
	Vector3 baseLookAt_;     // 注視点

	float shakeTime_ = 0.0f; // 揺れの時間経過
	float zoomTime_ = 0.0f;  // ズームの進行
	float totalTime_ = 2.0f; // ズーム演出全体の時間

	// === ランダムシェイク用 ===
	std::random_device seedGenerator_;
	std::mt19937 randomEngine_;
};

