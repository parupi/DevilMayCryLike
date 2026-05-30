#include "GameSceneStateClear.h"
#include "scene/GameScene/GameScene.h"

void GameSceneStateClear::Enter(GameScene& scene) {
	scene.GetInputContext()->SetCanPlayerMove(false);
	scene.GetInputContext()->SetCanLockOn(false);
	scene.GetInputContext()->SetCanCameraMove(false);
	scene.SetSceneTime(0.0f);
}

void GameSceneStateClear::Update(GameScene& scene) {
	scene;
}

void GameSceneStateClear::Exit(GameScene& scene) {
	scene;
}
