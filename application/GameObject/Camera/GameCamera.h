#pragma once
#include "Camera/Camera.h"
class GameCamera : public Camera
{
public:
	GameCamera(std::string cameraName);
	~GameCamera() override = default;

	void Update() override;
};

