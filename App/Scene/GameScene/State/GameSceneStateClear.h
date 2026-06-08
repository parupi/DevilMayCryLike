#pragma once
#include "GameSceneStateBase.h"

class GameSceneStateClear : public GameSceneStateBase {
public:
	GameSceneStateClear() = default;
	~GameSceneStateClear() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;

private:
	float waitTime_ = 0.0f;
	float waitDuration_ = 1.0f;
	bool requested_ = false;
};
