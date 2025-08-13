#pragma once
#include "3d/Camera/Camera.h"
#include "input/Input.h"
class Player;
class GameCamera : public Camera
{
public:
	GameCamera(std::string cameraName);
	~GameCamera() override = default;

	void Update() override;

private:
	Player* player_ = nullptr;
	Input* input_ = Input::GetInstance();

	float horizontalAngle_ = 0.0f; // 左右回転角（ラジアン）
};

