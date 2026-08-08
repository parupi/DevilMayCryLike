#pragma once
#include "Scene/GameScene/State/GameSceneStateBase.h"
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
		Enter,   // 暗転しながらメニューを出す
		Normal,  // 操作を受け付けている
		Leaving, // シーンの切り替えを要求済み。あとは暗転を待つだけ
	}menuState_ = MenuState::Enter;
};

