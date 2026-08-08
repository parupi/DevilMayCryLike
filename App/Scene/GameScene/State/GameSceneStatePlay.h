#pragma once
#include "Scene/GameScene/State/GameSceneStateBase.h"
class GameSceneStatePlay : public GameSceneStateBase
{
public:
	GameSceneStatePlay() = default;
	~GameSceneStatePlay() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;
private:
	// 戦闘中かどうかでBGMを差し替える
	void UpdateBattleBGM(GameScene& scene);

	enum class PlayState {
		Enter,
		Play,
	}state_ = PlayState::Enter;

	float muskAlpha_ = 0.0f;
	// チュートリアルを開始済みかどうか（メニューからの復帰時などに再発火させないため）
	bool tutorialStarted_ = false;
};

