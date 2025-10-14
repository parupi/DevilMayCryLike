#include "TitleCamera.h"
#include <base/utility/DeltaTime.h>
#include <algorithm>
#include <scene/Transition/SceneTransitionController.h>

TitleCamera::TitleCamera(std::string objectName) : Camera(objectName)
{
}

void TitleCamera::Initialize()
{
	transform_.translate = { 0.0f, 5.2f, -30.0f };
	transform_.rotate = { 0.15f, 0.0f, 0.0f };
}

void TitleCamera::Update()
{
	switch (titleState_) {
	case TitleState::Enter: {
		stateTimer_ += DeltaTime::GetDeltaTime();
		// 0.0f ～ 1.0f にクランプ
		float t = std::clamp(stateTimer_ / stateTime_, 0.0f, 1.0f);

		float easeT = t * (2 - t);
		transform_.translate = Lerp(startTranslate_, targetTranslate_, easeT);

		transform_.rotate = Lerp(startRotate_, targetRotate_, easeT);

		if (t >= 1.0f) {
			titleState_ = TitleState::Idle;
		}
		break;
	}
	case TitleState::Exit: {
		stateTimer_ += DeltaTime::GetDeltaTime();
		// 0.0f ～ 1.0f にクランプ
		float t = std::clamp(stateTimer_ / stateTime_, 0.0f, 1.0f);

		float easeT = t * t;
		transform_.translate = Lerp(startTranslate_, targetTranslate_, easeT);

		easeT = t * (2 - t);
		transform_.rotate = Lerp(startRotate_, targetRotate_, easeT);

		if (t >= 0.6f) {
			SceneTransitionController::GetInstance()->RequestSceneChange("GAMEPLAY", true);
		}
		break;
	}
	default:

		break;
	}

	Camera::Update();
}

void TitleCamera::Enter()
{
	targetTranslate_ = { 0.0f, 5.2f, -25.0f };
	targetRotate_ = { 0.25, 0.0f, 0.0f };
	startTranslate_ = transform_.translate;
	startRotate_ = transform_.rotate;
	stateTimer_ = 0.0f;
	stateTime_ = 30.0f;
	titleState_ = TitleState::Enter;
}

void TitleCamera::Exit()
{
	targetTranslate_ = { 0.0f, 5.2f, -5.5f };
	targetRotate_ = { 0.05f, 0.0f, 0.0f };
	startTranslate_ = transform_.translate;
	startRotate_ = transform_.rotate;
	stateTimer_ = 0.0f;
	stateTime_ = 1.5f;
	titleState_ = TitleState::Exit;
}
