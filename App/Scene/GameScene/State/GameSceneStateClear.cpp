#include "GameSceneStateClear.h"
#include "scene/GameScene/GameScene.h"
#include <scene/Transition/SceneTransitionController.h>
#include <base/utility/DeltaTime.h>

void GameSceneStateClear::Enter(GameScene& scene) {
	scene.GetInputContext()->SetCanPlayerMove(false);
	scene.GetInputContext()->SetCanLockOn(false);
	scene.GetInputContext()->SetCanCameraMove(false);
	scene.SetSceneTime(DeltaTime::GetDeltaTime() / 10.0f);
	waitTime_ = 0.0f;
	requested_ = false;
}

void GameSceneStateClear::Update(GameScene& scene) {
	if (requested_) return;

	waitTime_ += DeltaTime::GetDeltaTime();
	if (waitTime_ >= waitDuration_) {
		requested_ = true;
		SceneTransitionController::GetInstance()->RequestSceneChange("CLEAR", true);
	}
}

void GameSceneStateClear::Exit(GameScene& scene) {
	scene;
}
