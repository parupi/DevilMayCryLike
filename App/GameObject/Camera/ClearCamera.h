#pragma once
#include <3d/Camera/Camera.h>
#include <GameObject/Player/Player.h>
class ClearCamera : public Camera
{
public:
	ClearCamera(std::string cameraName);
	~ClearCamera() override = default;

	void Update() override;

private:

	Player* player_ = nullptr;

};

