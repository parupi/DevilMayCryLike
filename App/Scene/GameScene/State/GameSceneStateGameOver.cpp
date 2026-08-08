#include "GameSceneStateGameOver.h"
#include "Scene/GameScene/GameScene.h"

#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Scene/Transition/SceneTransitionController.h>
#include <Scene/Transition/TransitionManager.h>

void GameSceneStateGameOver::Enter(GameScene& scene) {
	requested_ = false;

	scene.GetInputContext()->SetCanPlayerMove(false);
	scene.GetInputContext()->SetCanLockOn(false);
	scene.GetInputContext()->SetCanCameraMove(false);

	// 死亡演出が HUD ごと隠しているので、選択肢を出すために戻す。
	// ゲーム中のHUDも一緒に出てくるが、暗幕の下になるので気にならない
	SpriteManager::GetInstance().SetUILayerVisible(true);

	scene.GetGameOverUI()->Enter();
}

void GameSceneStateGameOver::Update(GameScene& scene) {
	// 時間は止める。演出だけ実時間で動く
	scene.SetSceneTime(0.0f);

	if (requested_) return;

	switch (scene.GetGameOverUI()->GetResult()) {
	case GameOverUI::Result::Retry:
		requested_ = true;
		// 死亡演出のビネットをそのまま切り替えにも使う
		TransitionManager::GetInstance().SetTransition("Death");
		SceneTransitionController::GetInstance().RequestSceneChange("GAMEPLAY", true);
		break;
	case GameOverUI::Result::ToTitle:
		requested_ = true;
		TransitionManager::GetInstance().SetTransition("Fade");
		SceneTransitionController::GetInstance().RequestSceneChange("TITLE", true);
		break;
	default:
		break;
	}
}

void GameSceneStateGameOver::Exit(GameScene&) {
}
