#include "GameSceneStatePlay.h"
#include "Scene/GameScene/GameScene.h"
#include <Utility/DeltaTime.h>
#include <GameObject/Event/EventManager.h>
#include <GameObject/Event/ClearEvent.h>
#include <Input/Input.h>

void GameSceneStatePlay::Enter(GameScene& scene) {
	state_ = PlayState::Enter;
	scene.GetInputContext()->SetCanPlayerMove(true);
	scene.GetInputContext()->SetCanLockOn(true);
	scene.GetInputContext()->SetCanCameraMove(true);
}

void GameSceneStatePlay::Update(GameScene& scene) {
	scene.SetSceneTime(DeltaTime::GetDeltaTime());

	float maskAlpha = scene.GetMaskAlpha();

	switch (state_) {
	case PlayState::Enter:

		maskAlpha -= DeltaTime::GetDeltaTime() * 2.0f;

		if (maskAlpha < 0.0f) {
			maskAlpha = 0.0f;
			state_ = PlayState::Play;

			// 開始演出と被らないよう、演出が終わってからチュートリアルを開始する（一度だけ）
			if (!tutorialStarted_) {
				tutorialStarted_ = true;
				scene.GetTutorialService()->StartTutorial(TutorialState::Move);
			}
		}

		break;
	case PlayState::Play:

		if (Input::GetInstance().TriggerKey(DIK_M) || Input::GetInstance().PushButton(ButtonStart)) {
			scene.ChangeState("Menu");
		}

		{
			auto* clearEvent = static_cast<ClearEvent*>(EventManager::GetInstance().FindEvent("Event_Clear"));
			if (clearEvent && clearEvent->IsClear()) {
				scene.ChangeState("Clear");
			}
		}
		break;
	}
	scene.SetMaskAlpha(maskAlpha);
}

void GameSceneStatePlay::Exit(GameScene&) {}
