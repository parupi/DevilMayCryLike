#include "GameSceneStatePlay.h"
#include "Scene/GameScene/GameScene.h"
#include <Utility/DeltaTime.h>
#include <GameObject/Event/EventManager.h>
#include <GameObject/Event/ClearEvent.h>
#include <Input/Input.h>
#include <Audio/SoundManager.h>
#include <GameData/GameSettings.h>

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

			// 開始演出と被らないよう、演出が終わってからチュートリアルを開始する（一度だけ）。
			// 2周目以降のために、タイトルの OPTION から切れるようにしてある
			if (!tutorialStarted_) {
				tutorialStarted_ = true;
				if (GameSettings::GetInstance().IsTutorialEnabled()) {
					scene.GetTutorialService()->StartTutorial(TutorialState::Move);
				} else {
					// TutorialDummy は全チュートリアル完了まで死なないので、
					// 流さない場合は先に完了扱いにしておかないと進行が止まる
					scene.GetTutorialService()->SkipAllTutorials();
				}
			}
		}

		break;
	case PlayState::Play:

		UpdateBattleBGM(scene);

		if (Input::GetInstance().TriggerKey(DIK_M) || Input::GetInstance().PushButton(ButtonStart)) {
			scene.ChangeState("Menu");
		}

		// 死亡演出が終わっていたら、やり直しの選択肢を出す
		if (scene.GetPlayer() && scene.GetPlayer()->IsDeathFinished()) {
			scene.ChangeState("GameOver");
			break;
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

void GameSceneStatePlay::UpdateBattleBGM(GameScene& scene) {
	Player* player = scene.GetPlayer();
	if (!player) return;

	// StylishScoreManager の戦闘フラグをそのまま使う。
	// カメラ側の Battle 状態は敵を見ただけで立つので、BGM の判断には細かすぎる
	auto* score = player->GetScoreManager();
	const bool inBattle = (score && score->IsBattleActive());

	// 同じ曲なら PlayBGM は何もしないので、毎フレーム呼んで構わない
	SoundManager::GetInstance().PlayBGM(inBattle ? "BattleBGM" : "GamePlayBGM", 1.5f);
}
