#include "GameSceneStatePlay.h"
#include "Scene/GameScene/GameScene.h"
#include "GameObject/Character/Player/Player.h"
#include <Utility/DeltaTime.h>
#include <algorithm>
#include <GameObject/Event/EventManager.h>
#include <GameObject/Event/ClearEvent.h>
#include <Input/Input.h>
#include <Audio/SoundManager.h>
#include <GameData/GameSession.h>
#include <Scene/Transition/TransitionManager.h>
#include <Scene/Transition/SceneTransitionController.h>

void GameSceneStatePlay::Enter(GameScene& scene) {
	state_ = PlayState::Enter;
	scene.GetInputContext()->SetCanPlayerMove(true);
	scene.GetInputContext()->SetCanLockOn(true);
	scene.GetInputContext()->SetCanCameraMove(true);

	// ポーズを START で閉じて戻ってきた場合、その押下を長押しとして数えない
	if (TutorialSkipInput* skip = scene.GetTutorialSkip()) {
		skip->Reset();
	}
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
		// ジャスト回避の一瞬のスロー。避けた瞬間を認識させるためのもので、
		// 長さは Player 側が実時間で数えている（0.08秒程度）
		if (player) {
			worldScale = player->GetWorldTimeScale();
		}
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
			// 流すかどうかの判断は GameScene::Initialize が済ませている（トレーニング／OPTIONの設定）。
			// 流さないシーンでは StartTutorial が何もせず、最初から完了扱いになっている
			if (!tutorialStarted_) {
				tutorialStarted_ = true;
				scene.GetTutorialService()->StartTutorial(TutorialState::Move);
			}
		}

		break;
	case PlayState::Play:

		UpdateBattleBGM(scene);

		{
			const bool pauseRequested = UpdatePauseAndSkip(scene);

			// チュートリアルを終えた（またはスキップした）らボス戦の部屋へ。
			// 暗転が始まったら他の操作は受けない
			if (UpdateTutorialFinish(scene)) {
				break;
			}

			if (pauseRequested) {
				scene.ChangeState("Menu");
			}
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
			// ステージ上の名前は "ClearEvent" などエディタで付けたものなので、名前ではなく種類で引く
			// （以前は "Event_Clear" という名前で探していて見つからず、ボスを倒してもクリアしなかった）
			auto* clearEvent = static_cast<ClearEvent*>(EventManager::GetInstance().FindEventByType(EventType::Clear));
			if (clearEvent && clearEvent->IsClear()) {
				scene.ChangeState("Clear");
			}
		}
		break;
	}
	scene.SetMaskAlpha(maskAlpha);
}

void GameSceneStatePlay::Exit(GameScene&) {}

bool GameSceneStatePlay::UpdateTutorialFinish(GameScene& scene) {
	if (bossTransitionRequested_) return true;
	if (!GameSession::IsTutorialStage()) return false;

	// 最後の説明文と背景が消えきったところで完了になる。
	// OPTION でチュートリアルを切っている場合は最初から完了なので、開始演出のあとすぐ進む
	TutorialService* tutorial = scene.GetTutorialService();
	if (!tutorial || !tutorial->IsAllFinished()) return false;

	// 死亡演出中に切り替えると GameOver を飛ばしてしまう
	Player* player = scene.GetPlayer();
	if (player && player->IsDying()) return false;

	tutorialFinishTimer_ += DeltaTime::GetDeltaTime();
	if (tutorialFinishTimer_ < kTutorialFinishDelay) return false;

	RequestBossBattle();
	return true;
}

bool GameSceneStatePlay::UpdatePauseAndSkip(GameScene& scene) {
	TutorialSkipInput* skip = scene.GetTutorialSkip();
	if (!skip) {
		// チュートリアルの部屋以外は、押した瞬間にポーズを開く
		Input& input = Input::GetInstance();
		return input.TriggerKey(DIK_M) || input.PushButton(ButtonStart);
	}

	// チュートリアルの部屋では「短く押して離したらポーズ／押し続けたらスキップ」。
	// 死亡演出中と遷移を始めた後は数えない（ゲージは減っていく）
	Player* player = scene.GetPlayer();
	const bool canSkip = !bossTransitionRequested_ && !(player && player->IsDying());
	const TutorialSkipInput::Result result = skip->Update(DeltaTime::GetDeltaTime(), canSkip);

	if (result == TutorialSkipInput::Result::Skip) {
		if (TutorialSkipGauge* gauge = scene.GetTutorialSkipGauge()) {
			gauge->Flash();
		}
		// 待ち時間を挟まず、すぐに遷移を始める
		RequestBossBattle();
		return false;
	}
	return result == TutorialSkipInput::Result::Pause;
}

void GameSceneStatePlay::RequestBossBattle() {
	if (bossTransitionRequested_) return;
	bossTransitionRequested_ = true;
	// 読み直す前に行き先を切り替える（GameScene::Initialize が GameSession からステージを引く）
	GameSession::AdvanceToBossBattle();
	SoundManager::GetInstance().StopBGM(1.0f);
	TransitionManager::GetInstance().SetTransition("Fade");
	SceneTransitionController::GetInstance().RequestSceneChange("GAMEPLAY", true);
}

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
