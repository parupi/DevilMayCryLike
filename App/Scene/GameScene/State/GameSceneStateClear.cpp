#include "GameSceneStateClear.h"
#include "Scene/GameScene/GameScene.h"
#include <Scene/Transition/SceneTransitionController.h>
#include <Utility/DeltaTime.h>
#include <Audio/SoundManager.h>

void GameSceneStateClear::Enter(GameScene& scene) {
	// クリア演出に入ったら戦闘中のBGMを引かせる（CLEARシーンで ClearBGM に切り替わる）
	SoundManager::GetInstance().StopBGM(1.0f);
	scene.GetInputContext()->SetCanPlayerMove(false);
	scene.GetInputContext()->SetCanLockOn(false);
	scene.GetInputContext()->SetCanCameraMove(false);
	scene.SetSceneTime(DeltaTime::GetDeltaTime() / 10.0f);
	waitTime_ = 0.0f;
	requested_ = false;
}

void GameSceneStateClear::Update(GameScene&) {
	if (requested_) return;

	waitTime_ += DeltaTime::GetDeltaTime();
	if (waitTime_ >= waitDuration_) {
		requested_ = true;
		SceneTransitionController::GetInstance().RequestSceneChange("CLEAR", true);
	}
}

void GameSceneStateClear::Exit(GameScene&) {
}
