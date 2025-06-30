#pragma once
#include "3d/Camera/Camera.h"
class Player;
class GameCamera : public Camera
{
public:
	GameCamera(std::string cameraName);
	~GameCamera() override = default;

	void Update() override;

private:
	Player* player_ = nullptr;

	float horizontalAngle_ = 0.0f; // 左右回転角（ラジアン）
};

