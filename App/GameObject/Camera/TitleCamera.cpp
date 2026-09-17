#include "TitleCamera.h"
#include <Utility/DeltaTime.h>
#include <algorithm>
#include <cmath>
#include <Scene/Transition/SceneTransitionController.h>

TitleCamera::TitleCamera(std::string objectName) : BaseCamera(objectName)
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
			BeginIdle();
		}
		break;
	}
	case TitleState::Idle: {
		// 待機中は完全に静止させず、ごくわずかに漂わせる。
		// 軸ごとに周期をずらした sin を足しているので、繰り返しの継ぎ目が目に付きにくい
		idleTimer_ += DeltaTime::GetDeltaTime();

		transform_.translate.x = idleBaseTranslate_.x + std::sin(idleTimer_ * 0.23f) * 0.55f;
		transform_.translate.y = idleBaseTranslate_.y + std::sin(idleTimer_ * 0.37f) * 0.22f;
		transform_.translate.z = idleBaseTranslate_.z + std::sin(idleTimer_ * 0.17f) * 0.40f;

		transform_.rotate.x = idleBaseRotate_.x + std::sin(idleTimer_ * 0.31f) * 0.004f;
		transform_.rotate.y = idleBaseRotate_.y + std::sin(idleTimer_ * 0.19f) * 0.006f;
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

		// 飛び込み切る前に暗転を始めたいので、途中でシーン切り替えを要求する。
		// この条件は以降ずっと成立し続けるため、1回だけ通す
		if (!sceneChangeRequested_ && t >= kExitSceneChangeRate) {
			sceneChangeRequested_ = true;
			SceneTransitionController::GetInstance().RequestSceneChange("GAMEPLAY", true);
		}
		break;
	}
	default:

		break;
	}

	BaseCamera::Update();
}

void TitleCamera::Enter()
{
	targetTranslate_ = { 0.0f, 5.2f, -25.0f };
	targetRotate_ = { 0.18f, 0.0f, 0.0f };
	startTranslate_ = transform_.translate;
	startRotate_ = transform_.rotate;
	stateTimer_ = 0.0f;
	stateTime_ = kEnterTime;
	titleState_ = TitleState::Enter;
}

void TitleCamera::SkipEnter()
{
	if (titleState_ != TitleState::Enter) return;

	// 到達点まで飛ばしてから待機に入る
	transform_.translate = targetTranslate_;
	transform_.rotate = targetRotate_;
	BeginIdle();
}

void TitleCamera::BeginIdle()
{
	// 今いる場所を漂いの中心にする。
	// Enterを最後まで見た場合もスキップした場合も同じ位置に収まる
	idleBaseTranslate_ = transform_.translate;
	idleBaseRotate_ = transform_.rotate;
	idleTimer_ = 0.0f;
	titleState_ = TitleState::Idle;
}

void TitleCamera::Exit()
{
	targetTranslate_ = { 0.0f, 5.2f, -5.5f };
	targetRotate_ = { 0.05f, 0.0f, 0.0f };
	startTranslate_ = transform_.translate;
	startRotate_ = transform_.rotate;
	stateTimer_ = 0.0f;
	stateTime_ = kExitTime;
	titleState_ = TitleState::Exit;
	isExit_ = true;
	sceneChangeRequested_ = false;
}
