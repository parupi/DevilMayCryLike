#pragma once
#include "3d/Camera/BaseCamera.h"
#include <GameObject/Character/Player/Player.h>
class ClearCamera : public BaseCamera
{
public:
	ClearCamera(std::string cameraName);
	~ClearCamera() override = default;

	void Update() override;

private:

	Player* player_ = nullptr;

};

