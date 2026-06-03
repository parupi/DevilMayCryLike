#include "GameSceneStatePlay.h"
#include "Scene/GameScene/GameScene.h"
#include <Utility/DeltaTime.h>
#include <GameObject/Event/EventManager.h>
#include <GameObject/Event/ClearEvent.h>

void GameSceneStatePlay::Enter(GameScene& scene) {
	state_ = PlayState::Enter;
	scene.GetInputContext()->SetCanPlayerMove(true);
	scene.GetInputContext()->SetCanLockOn(true);
	scene.GetInputContext()->SetCanCameraMove(true);
}

void GameSceneStatePlay::Update(GameScene& scene) {
	scene.SetSceneTime(DeltaTime::GetDeltaTime());

	float muskAlpha = scene.GetMuskAlpha();

	switch (state_) {
	case PlayState::Enter:

		muskAlpha -= DeltaTime::GetDeltaTime() * 2.0f;

		if (muskAlpha < 0.0f) {
			muskAlpha = 0.0f;
			state_ = PlayState::Play;
		}

		break;
	case PlayState::Play:

		if (Input::GetInstance()->TriggerKey(DIK_M) || Input::GetInstance()->PushButton(ButtonStart)) {
			scene.ChangeState("Menu");
		}

		{
			auto* clearEvent = static_cast<ClearEvent*>(EventManager::GetInstance()->FindEvent("Event_Clear"));
			if (clearEvent && clearEvent->IsClear()) {
				scene.ChangeState("Clear");
			}
		}
		break;
	}
	scene.SetMuskAlpha(muskAlpha);
}

void GameSceneStatePlay::Exit(GameScene& scene) {}
