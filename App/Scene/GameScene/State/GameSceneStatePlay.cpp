#include "GameSceneStatePlay.h"
#include "Scene/GameScene/GameScene.h"
#include "GameObject/Character/Player/Player.h"
#include <Utility/DeltaTime.h>
#include <algorithm>
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
	// 死亡演出中は世界（敵・イベント）をスローにしてから止める。
	// 倒れるプレイヤーだけは Player::Update が実時間で動くので、ここで止まっても最後まで演じ切る
	float worldScale = 1.0f;
	Player* player = scene.GetPlayer();
	if (player && player->IsDying()) {
		deathWorldTimer_ += DeltaTime::GetDeltaTime();
		const float t = std::clamp(deathWorldTimer_ / kWorldStopDuration, 0.0f, 1.0f);
		worldScale = kWorldSlowScale * (1.0f - t);
	} else {
		deathWorldTimer_ = 0.0f;
	}
	scene.SetSceneTime(DeltaTime::GetDeltaTime() * worldScale);

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
				// トレーニングは操作の確認済みで入る場所なので流さない
				// （GameScene::Initialize が先に完了扱いにしている）
				if (!scene.IsTrainingMode() && GameSettings::GetInstance().IsTutorialEnabled()) {
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

		// トレーニングの設定メニュー。キーを見ているのは TrainingController なので、
		// ここは要求を受け取って状態を切り替えるだけ
		if (TrainingController* training = scene.GetTrainingController()) {
			if (training->ConsumeMenuRequest()) {
				scene.ChangeState("TrainingMenu");
				break;
			}
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
