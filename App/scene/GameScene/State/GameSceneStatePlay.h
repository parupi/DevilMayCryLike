#pragma once
#include "scene/GameScene/State/GameSceneStateBase.h"
class GameSceneStatePlay : public GameSceneStateBase
{
public:
	GameSceneStatePlay() = default;
	~GameSceneStatePlay() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;
private:
	enum class PlayState {
		Enter,
		Play,
	}state_ = PlayState::Enter;

	float muskAlpha_ = 0.0f;
};

