#pragma once
#include "scene/GameScene/State/GameSceneStateBase.h"
class GameSceneStateMenu : public GameSceneStateBase
{
public:
	GameSceneStateMenu() = default;
	~GameSceneStateMenu() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;

private:
	enum class MenuState {
		Enter,
		Normal,
	}menuState_ = MenuState::Enter;
};

