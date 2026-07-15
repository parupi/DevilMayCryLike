#include "InputContext.h"

void InputContext::Initialize(Input* input) {
	playerInput_ = std::make_unique<PlayerInput>();
	playerInput_->Initialize(input);

	lockOnInput_ = std::make_unique<LockOnInput>();
	lockOnInput_->Initialize(input);

	cameraInput_ = std::make_unique<CameraInput>();
	cameraInput_->Initialize(input);
}

void InputContext::Update() {
	if (canPlayerMove_) {
		playerInput_->Update();
	}
	if (canLockOn_) {
		lockOnInput_->Update();
	}
	if (canCameraMove_) {
		cameraInput_->Update();
	}
}
