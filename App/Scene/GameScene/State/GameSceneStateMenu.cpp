#include "GameSceneStateMenu.h"
#include "Scene/GameScene/GameScene.h"
#include "Input/Input.h"
#include <Utility/DeltaTime.h>

void GameSceneStateMenu::Enter(GameScene& scene)
{
	menuState_ = MenuState::Enter;
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
		maskAlpha += DeltaTime::GetDeltaTime() * 1.5f;

		if (maskAlpha >= 0.8f) {
			maskAlpha = 0.8f;
			scene.GetMenuUI()->Enter();
			menuState_ = MenuState::Normal;
		}
		break;
	case MenuState::Normal:
		if (Input::GetInstance().TriggerKey(DIK_M) || Input::GetInstance().TriggerButton(ButtonStart)) {
			scene.ChangeState("Play");
		}
		break;
	}
	scene.SetMaskAlpha(maskAlpha);
}

void GameSceneStateMenu::Exit(GameScene& scene)
{
	scene.GetMenuUI()->Exit();
}
