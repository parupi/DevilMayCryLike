#include "GameSceneStateMenu.h"
#include "Scene/GameScene/GameScene.h"
#include "Input/Input.h"
#include <Scene/Transition/SceneTransitionController.h>
#include <Scene/Transition/TransitionManager.h>
#include <Utility/DeltaTime.h>
#include <Audio/SoundManager.h>

void GameSceneStateMenu::Enter(GameScene& scene)
{
	menuState_ = MenuState::Enter;
	// ゲームが止まっている間はBGMも止める
	SoundManager::GetInstance().PauseBGM();
	scene.GetInputContext()->SetCanPlayerMove(false);
	scene.GetInputContext()->SetCanLockOn(false);
	scene.GetInputContext()->SetCanCameraMove(false);
}

void GameSceneStateMenu::Update(GameScene& scene)
{
	scene.SetSceneTime(0.0f);

	float maskAlpha = scene.GetMaskAlpha();

	switch (menuState_) {
	case MenuState::Enter:
		// 暗転は実時間で進める（この時点でシーンの時間は止めてある）
		maskAlpha += DeltaTime::GetUnscaledDeltaTime() * 1.5f;

		if (maskAlpha >= 0.8f) {
			maskAlpha = 0.8f;
			scene.GetMenuUI()->Enter();
			menuState_ = MenuState::Normal;
		}
		break;
	case MenuState::Normal:
		// 実際の分岐はメニュー側が決める。ここは結果を受けて動かすだけ
		switch (scene.GetMenuUI()->GetResult()) {
		case MenuUI::Result::Resume:
			scene.ChangeState("Play");
			break;
		case MenuUI::Result::Retry:
			// 同じシーンを読み直す。死亡時のやり直しと同じ経路
			TransitionManager::GetInstance().SetTransition("Fade");
			SceneTransitionController::GetInstance().RequestSceneChange("GAMEPLAY", true);
			menuState_ = MenuState::Leaving;
			break;
		case MenuUI::Result::ToTitle:
			TransitionManager::GetInstance().SetTransition("Fade");
			SceneTransitionController::GetInstance().RequestSceneChange("TITLE", true);
			menuState_ = MenuState::Leaving;
			break;
		default:
			break;
		}
		break;
	case MenuState::Leaving:
		// 暗転しきってシーンが切り替わるのを待つだけ
		break;
	}
	scene.SetMaskAlpha(maskAlpha);
}

void GameSceneStateMenu::Exit(GameScene& scene)
{
	SoundManager::GetInstance().ResumeBGM();
	scene.GetMenuUI()->Exit();
}
