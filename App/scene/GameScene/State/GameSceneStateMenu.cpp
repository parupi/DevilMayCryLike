#include "GameSceneStateMenu.h"
#include "scene/GameScene/GameScene.h"
#include "input/Input.h"

void GameSceneStateMenu::Enter(GameScene& scene)
{
	menuState_ = MenuState::Enter;
	scene.GetInputContext()->SetCanPlayerMove(false);
}

void GameSceneStateMenu::Update(GameScene& scene)
{
	scene.SetSceneTime(0.0f);

	float muskAlpha = scene.GetMuskAlpha();

	switch (menuState_) {
	case MenuState::Enter:
		muskAlpha += DeltaTime::GetDeltaTime() * 1.5f;

		if (muskAlpha >= 0.8f) {
			muskAlpha = 0.8f;
			scene.GetMenuUI()->Enter();
			menuState_ = MenuState::Normal;
		}
		break;
	case MenuState::Normal:
		if (Input::GetInstance()->TriggerKey(DIK_M)) {
			scene.ChangeState("Play");
		}
		break;
	}
	scene.SetMuskAlpha(muskAlpha);
}

void GameSceneStateMenu::Exit(GameScene& scene)
{
	scene.GetMenuUI()->Exit();
}
